#include <string>
#include <iostream>
#include <algorithm>

#include "IR.h"
#include "IRMutator.h"
#include "IRVisitor.h"
#include "IRPrinter.h"
#include "type.h"
#include "parse.h"
#include "IRGenerator.h"
#include "limits.h"

Type index_type = Type::int_scalar(32);
Type dtype;

std::vector<Expr> ins;
std::vector<Expr> outs;
std::vector<Stmt> body;  // main stmt

std::vector<std::pair<std::string, std::vector<size_t> > > var_dom; // var and related dom


Group IRGenerator(record& js) {

    ins.clear();
    outs.clear();
    body.clear();
    var_dom.clear();

    if (js.type == "float")	dtype = Type::float_scalar(32);
	if (js.type == "int")	dtype = Type::int_scalar(32);

    // build index and main_stmt
    for (size_t i = 0; i < js.vs.size(); i++) {
        IRVisitor visitor;
        js.vs[i].visit_stmt(&visitor);

        size_t left_index = 0;
        for (int j = 0; j < visitor.middle; j++) {
            left_index += visitor.all_var[j].second;
        }

        std::vector<string> index_name; // all index name
        std::vector<Expr> allIndex; // all index expr

        for (size_t j = 0; j < visitor.all_index.size(); j++) {
            auto pos = find(index_name.begin(), index_name.end(), visitor.all_index[j]);
            if (pos == index_name.end()) { // a new index
                index_name.push_back(visitor.all_index[j]);
            }
        }

        for (size_t j = 0; j < index_name.size(); j++) {
            std::string ind = index_name[j];
            // find the minimal range for the index
            int range = INT_MAX;
            size_t pos = 0;
            for (size_t k = 0; k < visitor.all_index.size(); k++) {
                if (ind == visitor.all_index[k]) {
                    if (visitor.all_dom[k] < range) {
                        range = visitor.all_dom[k];
                        pos = k;
                    }
                }
            }
            Expr dom_ = Dom::make(index_type, 0, range);
            if (pos < left_index) { // left part
                Expr index_ = Index::make(index_type, ind, dom_, IndexType::Spatial);
                allIndex.push_back(index_);
            }
            else { // right part
                Expr index_ = Index::make(index_type, ind, dom_, IndexType::Reduce);
                allIndex.push_back(index_);
            }
        }

        // bind if with stmt
        Stmt stmt = js.vs[i];
        Expr false_var = Var::make(index_type, "tmp", {}, {});
        Stmt false_stmt = Move::make(false_var, IntImm::make(index_type, 0), MoveType::LocalToLocal);
        for (size_t j = 0; j < visitor.ifExpr.size(); j++) {
            auto exp = visitor.ifExpr[j];
            Expr if_cond = Binary::make(index_type, BinaryOpType::And,  
                                        Compare::make(index_type, CompareOpType::LT, exp.first, IntImm::make(index_type, exp.second)),
                                        Compare::make(index_type, CompareOpType::GE, exp.first, IntImm::make(index_type, 0)));
            
            stmt = IfThenElse::make(if_cond, stmt, false_stmt);
        }
        body.push_back(LoopNest::make(allIndex, {stmt}));

        // add var and dom to var_dom for ins and outs
        int dom_pos = 0;
        for (size_t j = 0; j < visitor.all_var.size(); j++) {
            std::string varname = visitor.all_var[j].first;
            int domnum = visitor.all_var[j].second;
            std::vector<size_t> vardom;
            for (int k = dom_pos; k < dom_pos + domnum; k++) {
                vardom.push_back(visitor.all_valid_dom[k]);
            }

            var_dom.push_back(std::make_pair(varname, vardom));
            dom_pos += domnum;
        }
    }

    // build ins and outs
    for (size_t i = 0; i < js.in.size(); i++) {
        std::string name = js.in[i];
        for (auto var : var_dom) {
            if (var.first == name) {
                auto in_expr = Var::make(dtype, name, {}, var.second);
                ins.push_back(in_expr);
                break;
            }
        }
    }
    for (size_t i = 0; i < js.out.size(); i++) {
        std::string name = js.out[i];
        for (auto var : var_dom) {
            if (var.first == name) {
                auto out_expr = Var::make(dtype, name, {}, var.second);
                outs.push_back(out_expr);
                break;
            }
        }
    }

    Group kernel = Kernel::make(js.name, ins, outs, body, KernelType::CPU);

    // printer
    // IRPrinter printer;
    // std::string code = printer.print(kernel);

    // std::cout << code;

    // std::cout << "Success!\n";
    return kernel;

}

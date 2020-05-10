/*
 * MIT License
 * 
 * Copyright (c) 2020 Size Zheng

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "IRVisitor.h"

namespace Boost {

namespace Internal {


void IRVisitor::visit(Ref<const IntImm> op) {
    return;
}


void IRVisitor::visit(Ref<const UIntImm> op) {
    return;
}


void IRVisitor::visit(Ref<const FloatImm> op) {
    return;
}


void IRVisitor::visit(Ref<const StringImm> op) {
    return;
}


void IRVisitor::visit(Ref<const Unary> op) {
    (op->a).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Binary> op) {
    (op->a).visit_expr(this);
    (op->b).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Compare> op) {
    (op->a).visit_expr(this);
    (op->b).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Select> op) {
    (op->cond).visit_expr(this);
    (op->true_value).visit_expr(this);
    (op->false_value).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Call> op) {
    for (auto arg : op->args) {
        arg.visit_expr(this);
    }
    return;
}


void IRVisitor::visit(Ref<const Cast> op) {
    (op->val).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Ramp> op) {
    (op->base).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Var> op) {
    // scalar
    if (op->shape.size() == 1 && op->shape[0] == 1) {
        all_var.push_back(std::make_pair(op->name, 0));
        return;
    }
    size_t size_before = all_valid_dom.size();
    for (size_t i = 0; i < op->args.size(); i++) {
        auto arg = op->args[i];
        auto sh = op->shape[i];
        all_dom.push_back(sh);
        all_valid_dom.push_back(sh);
        complex = false;
        // index like i +/- j need bound check
        if (arg.node_type() != IRNodeType::Index) {
            complex = true;
            ifExpr.push_back(std::make_pair(arg, sh));
        }
        arg.visit_expr(this);
        if (complex)
            all_dom.pop_back();
    }
    all_var.push_back(std::make_pair(op->name, all_valid_dom.size() - size_before));
    return;
}


void IRVisitor::visit(Ref<const Dom> op) {
    (op->begin).visit_expr(this);
    (op->extent).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Index> op) {
    if (complex) {
        auto bk = all_dom.back();
        all_dom.push_back(bk);
    }
    all_index.push_back(op->name);
    (op->dom).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const LoopNest> op) {
    for (auto index : op->index_list) {
        index.visit_expr(this);
    }
    for (auto body : op->body_list) {
        body.visit_stmt(this);
    }
    return;
}


void IRVisitor::visit(Ref<const IfThenElse> op) {
    (op->cond).visit_expr(this);
    (op->true_case).visit_stmt(this);
    (op->false_case).visit_stmt(this);
    return;
}


void IRVisitor::visit(Ref<const Move> op) {
    (op->dst).visit_expr(this);
    middle = all_var.size();
    (op->src).visit_expr(this);
    return;
}


void IRVisitor::visit(Ref<const Kernel> op) {
    for (auto expr : op->inputs) {
        expr.visit_expr(this);
    }
    for (auto expr : op->outputs) {
        expr.visit_expr(this);
    }
    for (auto stmt : op->stmt_list) {
        stmt.visit_stmt(this);
    }
    return;
}



}  // namespace Internal

}  // namespace Boost


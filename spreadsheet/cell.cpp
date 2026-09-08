#include "cell.h"
#include "sheet.h" 
#include <cassert>
#include <iostream>
#include <string>
#include <optional>
class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue(const SheetInterface& sheet) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};
class Cell::EmptyImpl : public Cell::Impl {
public:

    CellInterface::Value GetValue(const SheetInterface& /*sheet*/) const override {
        return std::string();
    }
    std::string GetText() const override {
        return std::string();
    }
    std::vector<Position> GetReferencedCells() const override {
        return std::vector<Position>{};
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {}

    CellInterface::Value GetValue(const SheetInterface& /*sheet*/) const override {
        if (text_.size() > 1 && text_.front() == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        else {
            return text_;
        }
    }
    std::string GetText() const override {
        return text_;
    }
    std::vector<Position> GetReferencedCells() const override {
        return std::vector<Position>{};
    }
private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(std::string expression)
        : formula_(ParseFormula(std::move(expression))) {
    }

    CellInterface::Value GetValue(const SheetInterface& sheet) const override {
        FormulaInterface::Value result = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        else {
            return std::get<FormulaError>(result);
        }
    }
    std::string GetText() const override {
        return std::string(1, FORMULA_SIGN) + formula_->GetExpression();
    }
    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(Sheet& sheet, Position pos)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet)
    , pos_(pos){
}

Cell::~Cell() {
    dependents_.clear();
}

void Cell::Set(std::string text) {

    if (text == GetText()) {
        return;
    }

    std::vector<Position> new_refs;

    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        std::unique_ptr<FormulaInterface> temp_formula = ParseFormula(text.substr(1));
        new_refs = temp_formula->GetReferencedCells();
    }

    if (HasCircularDependency(new_refs)) {
        throw CircularDependencyException("Circular Dependency");
    }

    std::vector<Position>old_refs = this->GetReferencedCells();
    for (Position& old_ref : old_refs) {
        sheet_.GetConcreteCell(old_ref)->RemoveDependent(this);
    }

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(text.substr(1));
    }
    else {
        impl_ = std::make_unique<TextImpl>(text);
    }

    for (Position& new_ref : new_refs) {
        sheet_.FindOrCreateCell(new_ref)->AddDependent(this);
    }
    std::unordered_set<Cell*> visited;
    InvalidateCacheRecursively(visited);

}

void Cell::Clear() {

    Set("");
}


Cell::Value Cell::GetValue() const
{
    if (cache_.has_value()) {
        return *cache_;
    }
    cache_ = impl_->GetValue(sheet_);
    return *cache_;

}
std::string Cell::GetText() const {
    return impl_->GetText();
}

const std::unordered_set<Cell*>& Cell::GetDependents() const
{
    return dependents_;
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

void Cell::AddDependent(Cell* dependent)
{
    dependents_.insert(dependent);
}

void Cell::RemoveDependent(Cell* dependent)
{
    dependents_.erase(dependent);
}

bool Cell::IsReferenced() const
{
    return !dependents_.empty();
}

void Cell::InvalidateCache()
{
    cache_.reset();
}

void Cell::InvalidateCacheRecursively(std::unordered_set<Cell*>& visited)
{
    if (visited.find(this) != visited.end()) {
        return;
    }
    visited.insert(this);
    InvalidateCache();
    const std::unordered_set<Cell*>& dependents = GetDependents();

    for (Cell* dep : dependents) {
        dep->InvalidateCacheRecursively(visited);
    }
}

bool Cell::DFS(Position current, std::unordered_set<Position, CellHasher>& visited) const
{
    if (current == this->pos_) {
        return true;
    }
    if (visited.find(current) != visited.end()) {
        return false;
    }
    visited.insert(current);
    const Cell* cell = sheet_.GetConcreteCell(current);
    if (cell == nullptr) {
        return false;
    }
    for (const Position& r : cell->GetReferencedCells()) {
        if (this->DFS(r, visited)) {
            return true;
        }
    }
    return false;
}

bool Cell::HasCircularDependency( const std::vector<Position>& new_refs) const
{
    std::unordered_set<Position, CellHasher> visited;
    for (const Position& ref : new_refs) {
        if (this->DFS(ref, visited)) {
            return true;
        }
    }
    return false;
}

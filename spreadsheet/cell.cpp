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

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() {
    dependents_.clear();
}

void Cell::Set(std::string text) {
    if (text == GetText()) {
        return;
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

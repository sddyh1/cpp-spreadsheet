#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}



void Sheet::SetCell(Position pos, std::string text) {
    ValidatePosition(pos);

    std::vector<Position> new_refs;
    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        std::unique_ptr<FormulaInterface> temp_formula = ParseFormula(text.substr(1));
        new_refs = temp_formula->GetReferencedCells();
    }

    if (HasCircularDependency(pos, new_refs)) {
        throw CircularDependencyException("Circular Dependency");
    }

    Cell* cell = FindOrCreateCell(pos);
    std::vector<Position>old_refs = cell->GetReferencedCells();
    for (Position& old_ref : old_refs) {
        GetConcreteCell(old_ref)->RemoveDependent(cell);
    }
    cell->Set(text);

    for (Position& new_ref : new_refs) {
        FindOrCreateCell(new_ref)->AddDependent(cell);
    }
    std::unordered_set<Cell*> visited;
    InvalidateCacheRecursively(cell, visited);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    ValidatePosition(pos);

    auto it = sheet_.find(pos);

    if (it == sheet_.end()) {
        return nullptr;
    }
    return it->second.get();

}
CellInterface* Sheet::GetCell(Position pos) {
    ValidatePosition(pos);

    auto it = sheet_.find(pos);

    if (it == sheet_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void Sheet::ClearCell(Position pos) {
    ValidatePosition(pos);

    Cell* cell = GetConcreteCell(pos);
    if (cell == nullptr) {
        return;
    }
    std::vector<Position> old_refs = cell->GetReferencedCells();
    for (const Position& old_ref : old_refs) {
        GetConcreteCell(old_ref)->RemoveDependent(cell);
    }
    cell->Clear();

    std::unordered_set<Cell*> visited;
    InvalidateCacheRecursively(cell, visited);

    sheet_.erase(pos);
}

Size Sheet::GetPrintableSize() const {
    int max_row = -1;
    int max_col = -1;

    for (const auto& [pos, cell] : sheet_) {
        if (!cell->GetText().empty()) {
            if (pos.row > max_row) {
                max_row = pos.row;
            }
            if (pos.col > max_col) {
                max_col = pos.col;
            }
        }

    }
    return Size{ max_row + 1, max_col + 1 };
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const CellInterface* cell = GetCell(Position{ row,col });
            if (cell != nullptr) {
                std::visit([&output](const auto& value) {
                    output << value;
                    }, cell->GetValue());
            }
        }
        output << "\n";
    }

}
void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const CellInterface* cell = GetCell(Position{ row,col });
            if (cell != nullptr) {
                output << cell->GetText();
            }
        }
        output << "\n";
    }
}

Cell* Sheet::FindOrCreateCell(Position pos)
{
    std::unique_ptr<Cell>& cell = sheet_[pos];

    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    return cell.get();
}

Cell* Sheet::GetConcreteCell(Position pos)
{
    auto it = sheet_.find(pos);
    if (it != sheet_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Sheet::InvalidateCacheRecursively(Cell* cell, std::unordered_set<Cell*>& visited) const
{
    if (visited.find(cell) != visited.end()) {
        return;
    }
    visited.insert(cell);
    cell->InvalidateCache();
    const std::unordered_set<Cell*>& dependents = cell->GetDependents();

    for (Cell* dep : dependents) {
        InvalidateCacheRecursively(dep, visited);
    }
}

bool Sheet::HasCircularDependency(Position pos, const std::vector<Position>& new_refs) const
{
    std::unordered_set<Position, CellHasher> visited;
    for (const Position& ref : new_refs) {
        if (DFS(ref, pos, visited)) {
            return true;
        }
    }
    return false;
}

bool Sheet::DFS(Position current, Position target, std::unordered_set<Position, CellHasher>& visited) const
{
    if (current == target) {
        return true;
    }
    if (visited.find(current) != visited.end()) {
        return false;
    }
    visited.insert(current);
    const CellInterface* cell = GetCell(current);
    if (cell == nullptr) {
        return false;
    }
    for (const Position& r : cell->GetReferencedCells()) {
        if (DFS(r, target, visited)) {
            return true;
        }
    }
    return false;
}
void Sheet::ValidatePosition(Position pos) const
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
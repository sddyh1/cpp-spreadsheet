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
    FindOrCreateCell(pos)->Set(text);
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
    cell->Clear();
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
        cell = std::make_unique<Cell>(*this, pos);
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

void Sheet::ValidatePosition(Position pos) const
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
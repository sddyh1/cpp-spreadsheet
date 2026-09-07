#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {}
        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(sheet);
            }
            catch (const FormulaError& e) {
                return e;
            }
        }
        std::string GetExpression() const override {
            std::ostringstream os;
            ast_.PrintFormula(os);
            return os.str();
        }
        std::vector<Position> GetReferencedCells() const override {
            return ast_.GetReferencedCells();
        }
    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
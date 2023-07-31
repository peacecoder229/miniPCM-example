#include <string>
#include <map>
#include <iostream>
#include "exprtk.hpp"

int main() {
    std::string formula = "c0*2*c2";
    std::string formula2 = "(p0 / p2) + 2*2*p3";
    double c0 = 20000000004.0;
    double c2 = 5.0;
    std::map<std::string, double> input;
    input["p0"] = 20;
    input["p2"] = 5;
    input["p1"] = 0;
    input["p3"] = 1;

    exprtk::symbol_table<double> symbol_table;
    symbol_table.add_variable("c0", c0);
    symbol_table.add_variable("c2", c2);

    exprtk::expression<double> expression;
    expression.register_symbol_table(symbol_table);

    exprtk::parser<double> parser;
    if (parser.compile(formula, expression))
    {
	std::cout << "Formula: " << formula << std::endl;
        std::cout << "Expression result: " << expression.value() << "\n";
    }
    else
    {
        std::cerr << "Failed to compile expression.\n";
    }
    for ( auto & pair : input)
    {
	    symbol_table.add_variable(pair.first, pair.second);
    }

    if (parser.compile(formula2, expression))
    {
	std::cout << "Formula2: " << formula2 << std::endl;
        std::cout << "Expression result: " << expression.value() << "\n";
    }
    else
    {
        std::cerr << "Failed to compile expression.\n";
    }


    return 0;
}


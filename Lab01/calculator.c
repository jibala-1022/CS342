#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to calculate the power of a number
double power(double base, double exponent) {
    double result = 1.0;
    
    // If the exponent is non-negative
    if (exponent >= 0) {
        for (int i = 0; i < exponent; ++i) {
            result *= base;
        }
    }
    // If the exponent is negative
    else {
        for (int i = 0; i > exponent; --i) {
            result /= base;
        }
    }
    return result;
}

// Function to evaluate a mathematical expression
char *calculator_evaluate(char *buffer) {
    double operand1, operand2;
    char operator;
    char *result = (char *) malloc(50 * sizeof(char)); // Allocate memory for the result

    // Parse the expression from the buffer
    if (sscanf(buffer, "%lf %c %lf", &operand1, &operator, &operand2) != 3) {
        sprintf(result, "Invalid expression"); // In case parsing fails
        return result;
    }

    // Evaluate the expression based on the operator
    switch (operator) {
        case '+':
            sprintf(result, "%lf", operand1 + operand2);
            break;
        case '-':
            sprintf(result, "%lf", operand1 - operand2);
            break;
        case '*':
            sprintf(result, "%lf", operand1 * operand2);
            break;
        case '/':
            if (operand2 != 0) {
                sprintf(result, "%lf", operand1 / operand2);
            } else {
                sprintf(result, "Division by zero");
            }
            break;
        case '^':
            sprintf(result, "%lf", power(operand1, operand2)); // Use the power function
            break;
        default:
            sprintf(result, "Unsupported operator");
            break;
    }
    return result;
}

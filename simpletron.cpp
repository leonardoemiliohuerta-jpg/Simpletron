/*
 
 *  Códigos de operación soportados:
 *      10  READ           Leer una palabra del teclado hacia una posición
 *      11  WRITE          Escribir una palabra desde una posición a pantalla
 *      20  LOAD           Cargar una palabra desde memoria al acumulador
 *      21  STORE          Almacenar una palabra del acumulador en memoria
 *      30  ADD            Sumar una palabra de memoria al acumulador
 *      31  SUBTRACT       Restar una palabra de memoria al acumulador
 *      32  DIVIDE         Dividir el acumulador entre una palabra de memoria
 *      33  MULTIPLY       Multiplicar una palabra de memoria al acumulador
 *      40  BRANCH         Salto incondicional a una posición de memoria
 *      41  BRANCHNEG      Salto si el acumulador es negativo
 *      42  BRANCHZERO     Salto si el acumulador es cero
 *      43  HALT           Detener la ejecución del programa
 *
 *  Autor: (Leonardo)
 * ============================================================================
 */

#include <iostream>
#include <iomanip>
#include <array>
#include <string>
#include <limits>
#include <cstdlib>

class Simpletron {
public:
    static const int MEMORY_SIZE = 100;
    static const int MEMORY_MIN = -9999;
    static const int MEMORY_MAX = 9999;

    // Códigos de operación
    enum OperationCode {
        READ       = 10,
        WRITE      = 11,
        LOAD       = 20,
        STORE      = 21,
        ADD        = 30,
        SUBTRACT   = 31,
        DIVIDE     = 32,
        MULTIPLY   = 33,
        BRANCH     = 40,
        BRANCHNEG  = 41,
        BRANCHZERO = 42,
        HALT       = 43
    };

    Simpletron()
        : memory{},
          accumulator(0),
          instructionCounter(0),
          instructionRegister(0),
          operationCode(0),
          operand(0),
          isRunning(true) {}

    // ----------------------------------------------------------------------
    // Fase de carga: lee el programa SML introducido por el usuario y lo
    // almacena en memoria a partir de la posición 00.
    // ----------------------------------------------------------------------
    bool load() {
        std::cout << "*** Bienvenido al simulador Simpletron ***\n";
        std::cout << "*** Ingrese su programa en Simpletron Machine Language (SML) ***\n";
        std::cout << "*** una instruccion (o palabra de datos) por linea.        ***\n";
        std::cout << "*** Escriba -99999 para finalizar la introduccion del      ***\n";
        std::cout << "*** programa.                                              ***\n\n";

        int address = 0;
        int instruction = 0;

        while (address < MEMORY_SIZE) {
            std::cout << std::setw(2) << std::setfill('0') << address << " ? ";

            if (!(std::cin >> instruction)) {
                std::cerr << "*** Entrada invalida. Intente de nuevo. ***\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            if (instruction == -99999) {
                break;
            }

            if (instruction < MEMORY_MIN || instruction > MEMORY_MAX) {
                std::cerr << "*** El valor debe estar entre " << MEMORY_MIN
                          << " y " << MEMORY_MAX << ". Intente de nuevo. ***\n";
                continue;
            }

            memory[address] = instruction;
            ++address;
        }

        std::cout << "\n*** El programa se ha cargado en memoria correctamente ***\n\n";
        return true;
    }

    // ----------------------------------------------------------------------
    // Fase de ejecución: recorre la memoria ejecutando cada instrucción
    // hasta encontrar HALT, un error, o el fin de la memoria.
    // ----------------------------------------------------------------------
    void execute() {
        std::cout << "*** Inicio de la ejecucion ***\n\n";

        while (isRunning) {
            if (instructionCounter < 0 || instructionCounter >= MEMORY_SIZE) {
                reportError("El contador de instruccion salio de rango de memoria.");
                dumpCore();
                return;
            }

            instructionRegister = memory[instructionCounter];
            operationCode = instructionRegister / 100;              // primeros 2 digitos
            operand = instructionRegister % 100;                    // ultimos 2 digitos
            if (operand < 0) operand = -operand;

            switch (operationCode) {
                case READ:
                    if (!doRead()) return;
                    break;
                case WRITE:
                    if (!doWrite()) return;
                    break;
                case LOAD:
                    if (!doLoad()) return;
                    break;
                case STORE:
                    if (!doStore()) return;
                    break;
                case ADD:
                    if (!doAdd()) return;
                    break;
                case SUBTRACT:
                    if (!doSubtract()) return;
                    break;
                case DIVIDE:
                    if (!doDivide()) return;
                    break;
                case MULTIPLY:
                    if (!doMultiply()) return;
                    break;
                case BRANCH:
                    instructionCounter = operand;
                    continue; // no incrementar el contador
                case BRANCHNEG:
                    if (accumulator < 0) {
                        instructionCounter = operand;
                        continue;
                    }
                    break;
                case BRANCHZERO:
                    if (accumulator == 0) {
                        instructionCounter = operand;
                        continue;
                    }
                    break;
                case HALT:
                    std::cout << "*** Fin de la ejecucion (HALT) ***\n\n";
                    isRunning = false;
                    break;
                default:
                    reportError("Codigo de operacion invalido.");
                    dumpCore();
                    return;
            }

            ++instructionCounter;
        }

        std::cout << "*** Ejecucion terminada exitosamente ***\n\n";
        dumpCore();
    }

private:
    std::array<int, MEMORY_SIZE> memory;
    int accumulator;
    int instructionCounter;
    int instructionRegister;
    int operationCode;
    int operand;
    bool isRunning;

    bool checkAddress(int address) {
        if (address < 0 || address >= MEMORY_SIZE) {
            reportError("Direccion de memoria fuera de rango.");
            dumpCore();
            return false;
        }
        return true;
    }

    bool checkOverflow(long value) {
        if (value < MEMORY_MIN || value > MEMORY_MAX) {
            reportError("Desbordamiento del acumulador (overflow).");
            dumpCore();
            return false;
        }
        return true;
    }

    bool doRead() {
        if (!checkAddress(operand)) return false;
        int value;
        std::cout << "Introduzca un entero (direccion " << std::setw(2)
                  << std::setfill('0') << operand << ") -> ";
        while (!(std::cin >> value) || value < MEMORY_MIN || value > MEMORY_MAX) {
            std::cerr << "*** Entrada invalida. Debe ser un entero entre "
                      << MEMORY_MIN << " y " << MEMORY_MAX << ". Intente de nuevo. ***\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Introduzca un entero (direccion " << std::setw(2)
                      << std::setfill('0') << operand << ") -> ";
        }
        memory[operand] = value;
        return true;
    }

    bool doWrite() {
        if (!checkAddress(operand)) return false;
        std::cout << "Contenido de la direccion " << std::setw(2) << std::setfill('0')
                  << operand << " -> " << memory[operand] << '\n';
        return true;
    }

    bool doLoad() {
        if (!checkAddress(operand)) return false;
        accumulator = memory[operand];
        return true;
    }

    bool doStore() {
        if (!checkAddress(operand)) return false;
        memory[operand] = accumulator;
        return true;
    }

    bool doAdd() {
        if (!checkAddress(operand)) return false;
        long result = static_cast<long>(accumulator) + memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = static_cast<int>(result);
        return true;
    }

    bool doSubtract() {
        if (!checkAddress(operand)) return false;
        long result = static_cast<long>(accumulator) - memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = static_cast<int>(result);
        return true;
    }

    bool doMultiply() {
        if (!checkAddress(operand)) return false;
        long result = static_cast<long>(accumulator) * memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = static_cast<int>(result);
        return true;
    }

    bool doDivide() {
        if (!checkAddress(operand)) return false;
        if (memory[operand] == 0) {
            reportError("Intento de division entre cero.");
            dumpCore();
            return false;
        }
        long result = static_cast<long>(accumulator) / memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = static_cast<int>(result);
        return true;
    }

    void reportError(const std::string &message) {
        std::cerr << "\n*** Error en tiempo de ejecucion: " << message << " ***\n";
        std::cerr << "*** Simpletron finalizando la ejecucion. ***\n\n";
    }

    // ----------------------------------------------------------------------
    // Volcado de memoria (core dump): muestra el estado completo del
    // registro y de la memoria, tal como se pide en un simulador clasico.
    // ----------------------------------------------------------------------
    void dumpCore() {
        std::cout << std::setfill(' '); // evitar que el relleno '0' de otras
                                         // impresiones se "pegue" al stream
        std::cout << "\nREGISTROS:\n";
        std::cout << "Acumulador             " << formatWord(accumulator) << '\n';
        std::cout << "contador de instruccion       "
                  << std::setw(2) << std::setfill('0') << instructionCounter << '\n';
        std::cout << "registro de instruccion     " << formatWord(instructionRegister) << '\n';
        std::cout << "codigo de operacion             "
                  << std::setw(2) << std::setfill('0') << operationCode << '\n';
        std::cout << "operando                      "
                  << std::setw(2) << std::setfill('0') << operand << "\n\n";

        std::cout << "MEMORIA:\n";
        std::cout << "     ";
        for (int col = 0; col < 10; ++col) {
            std::cout << std::setfill(' ') << std::setw(6) << col;
        }
        std::cout << '\n';

        for (int row = 0; row < MEMORY_SIZE / 10; ++row) {
            std::cout << std::setfill('0') << std::setw(2) << row * 10 << std::setfill(' ') << " ";
            for (int col = 0; col < 10; ++col) {
                std::cout << std::setw(6) << formatWord(memory[row * 10 + col]);
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }

    // Formatea una palabra con signo y 4 digitos, ej: +0042, -0007
    static std::string formatWord(int value) {
        std::string sign = (value < 0) ? "-" : "+";
        int absValue = std::abs(value);
        std::string digits = std::to_string(absValue);
        while (digits.size() < 4) {
            digits = "0" + digits;
        }
        return sign + digits;
    }
};

int main() {
    Simpletron sml;
    sml.load();
    sml.execute();
    return 0;
}

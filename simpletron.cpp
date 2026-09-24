/*
 * ============================================================================
 *  SIMPLETRON - Simulador de la Computadora Simpletron (SML)
 * ============================================================================
 *  Materia: PROGRAMACION AVANZADA 101-142-TCD202-011
 *  Documento de referencia: "2. Un simulador de Computadora"
 *  (Tomado de Deitel H.M. y Deitel P.J. (1998). Como programar en C,
 *   1ra Edicion. Mexico. Prentice Hall Latinoamerica. Usado con propositos
 *   educativos.)
 *
 *  Requisitos verificables cubiertos:
 *   1. Memoria de 100 posiciones inicializada correctamente.
 *   2. Captura de instrucciones hasta recibir 9999 (centinela).
 *   3. Validacion de las palabras introducidas durante la carga
 *      (rango permitido: -9999 a +9998; 9999 queda reservado como
 *      centinela y por eso NO es un valor de dato valido).
 *   4. Registros accumulator, instructionCounter, instructionRegister,
 *      operationCode y operand.
 *   5. Ciclo completo de busqueda, decodificacion y ejecucion.
 *   6. Implementacion de las doce operaciones SML indicadas.
 *   7. Vaciado formateado de todos los registros y de la memoria completa.
 *   8. Deteccion de division entre cero, codigo de operacion invalido,
 *      desbordamiento del acumulador (resultado > +9999 o < -9999) y
 *      otros errores fatales pertinentes.
 *   9. Ejecucion comprobable de programas SML correctos y de casos que
 *      provoquen errores fatales (ver ejemplos_prueba/ en el repositorio).
 *
 *  Codigos de operacion (las doce indicadas):
 *      10  READ            30  ADD
 *      11  WRITE           31  SUBTRACT
 *      20  LOAD            32  DIVIDE
 *      21  STORE           33  MULTIPLY
 *                          40  BRANCH
 *                          41  BRANCHNEG
 *                          42  BRANCHZERO
 *                          43  HALT
 *
 *  Formato de instruccion: ±DDDD (2 digitos de operacion + 2 de operando)
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

    // (2)/(3) Centinela y rango valido durante la carga del programa.
    static const int SENTINEL = 9999;
    static const int LOAD_MIN = -9999;
    static const int LOAD_MAX = 9998; // 9999 esta reservado como centinela

    // (8) Cota de desbordamiento para resultados aritmeticos.
    static const int OVERFLOW_MIN = -9999;
    static const int OVERFLOW_MAX = 9999;

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

    // (1) Memoria de 100 posiciones inicializada correctamente (a 0 por
    // el inicializador de std::array{}).
    Simpletron()
        : memory{},
          accumulator(0),
          instructionCounter(0),
          instructionRegister(0),
          operationCode(0),
          operand(0),
          isRunning(true) {}

    // ----------------------------------------------------------------------
    // (2)(3) Fase de carga: lee el programa SML introducido por el usuario,
    // valida cada palabra y detiene la carga al recibir el centinela 9999.
    // ----------------------------------------------------------------------
    void load() {
        std::cout << "*** Bienvenido a Simpletron! ***\n";
        std::cout << "*** Introduzca su programa una instruccion ***\n";
        std::cout << "*** (o palabra de datos) a la vez en la linea ***\n";
        std::cout << "*** de texto de entrada. Yo indicare el numero ***\n";
        std::cout << "*** de posicion y una interrogacion (?). Usted ***\n";
        std::cout << "*** tecleara entonces la palabra para esa ***\n";
        std::cout << "*** posicion. Escriba 9999 para terminar de ***\n";
        std::cout << "*** introducir su programa. ***\n\n";

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

            if (instruction == SENTINEL) {
                break;
            }

            // (3) Validacion: reintentar hasta capturar un numero valido.
            if (instruction < LOAD_MIN || instruction > LOAD_MAX) {
                std::cerr << "*** El valor debe estar en el intervalo ["
                          << LOAD_MIN << ", " << LOAD_MAX
                          << "]. Intente de nuevo. ***\n";
                continue;
            }

            memory[address] = instruction;
            ++address;
        }

        std::cout << "\n*** Se termino de cargar el programa ***\n";
        std::cout << "*** Comienza la ejecucion del programa ***\n\n";
    }

    // ----------------------------------------------------------------------
    // (5) Ciclo de busqueda, decodificacion y ejecucion.
    // ----------------------------------------------------------------------
    void execute() {
        while (isRunning) {
            if (instructionCounter < 0 || instructionCounter >= MEMORY_SIZE) {
                reportError("El contador de instruccion salio de rango de memoria.");
                dumpCore();
                return;
            }

            // Busqueda (fetch)
            instructionRegister = memory[instructionCounter];

            // Decodificacion (decode): separar OP y ADDR
            operationCode = instructionRegister / 100;
            operand = instructionRegister % 100;
            if (operand < 0) operand = -operand;

            // Ejecucion (execute)
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
                    std::cout << "*** Termino la ejecucion de Simpletron ***\n\n";
                    isRunning = false;
                    break;
                default:
                    reportError("Codigo de operacion invalido.");
                    dumpCore();
                    return;
            }

            ++instructionCounter;
        }

        // (7)(9) Vaciado tras una terminacion normal (HALT).
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

    // (8) Deteccion de desbordamiento del acumulador.
    bool checkOverflow(long value) {
        if (value < OVERFLOW_MIN || value > OVERFLOW_MAX) {
            reportError("Desbordamiento del acumulador (overflow).");
            dumpCore();
            return false;
        }
        return true;
    }

    // ------------------------------------------------------------------
    // (6) Las doce operaciones SML
    // ------------------------------------------------------------------
    bool doRead() {
        if (!checkAddress(operand)) return false;
        int value;
        std::cout << "Introduzca un entero (direccion " << std::setw(2)
                  << std::setfill('0') << operand << ") -> ";
        while (!(std::cin >> value) || value < LOAD_MIN || value > SENTINEL) {
            std::cerr << "*** Entrada invalida. Intente de nuevo. ***\n";
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
            reportError("Intento de dividir entre cero.");
            dumpCore();
            return false;
        }
        long result = static_cast<long>(accumulator) / memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = static_cast<int>(result);
        return true;
    }

    void reportError(const std::string &message) {
        std::cerr << "\n*** " << message << " ***\n";
        std::cerr << "*** La ejecucion de Simpletron termino anormalmente ***\n\n";
    }

    // Formatea una palabra con signo y 4 digitos: +0042, -0007
    static std::string formatWord(int value) {
        std::string sign = (value < 0) ? "-" : "+";
        int absValue = std::abs(value);
        std::string digits = std::to_string(absValue);
        while (digits.size() < 4) {
            digits = "0" + digits;
        }
        return sign + digits;
    }

    // ------------------------------------------------------------------
    // (7) Vaciado (dump) de todos los registros y de la memoria completa,
    // con el mismo formato mostrado en el documento de referencia:
    // encabezado de columnas 0-9, filas 0-9 (decena de la direccion).
    // ------------------------------------------------------------------
    void dumpCore() {
        std::cout << "Registros:\n";
        std::cout << "acumulador:                    " << formatWord(accumulator) << '\n';
        std::cout << "instructionCounter:                   "
                  << std::setw(2) << std::setfill('0') << instructionCounter << '\n';
        std::cout << std::setfill(' ');
        std::cout << "instructionRegister:               " << formatWord(instructionRegister) << '\n';
        std::cout << "operationcode:                          "
                  << std::setw(2) << std::setfill('0') << operationCode << '\n';
        std::cout << std::setfill(' ');
        std::cout << "operand:                                "
                  << std::setw(2) << std::setfill('0') << operand << "\n\n";
        std::cout << std::setfill(' ');

        std::cout << "MEMORIA\n";
        std::cout << "    ";
        for (int col = 0; col < 10; ++col) {
            std::cout << std::setw(7) << col;
        }
        std::cout << '\n';

        for (int row = 0; row < MEMORY_SIZE / 10; ++row) {
            std::cout << std::setw(2) << row << "  ";
            for (int col = 0; col < 10; ++col) {
                std::cout << std::setw(7) << formatWord(memory[row * 10 + col]);
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }
};

int main() {
    Simpletron sml;
    sml.load();
    sml.execute();
    return 0;
}
/*

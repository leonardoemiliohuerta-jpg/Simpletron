/*
====================================================
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <array>
#include <string>
#include <cmath>
#include <cctype>
#include <limits>
#include <cstdlib>

class Simpletron {
public:
    // ---- (2) Memoria ampliada a 1000 posiciones -----------------------------
    static const int MEMORY_SIZE = 1000;
    static constexpr double MEMORY_MAX_MAGNITUDE = 999999.0; // cota documentada

    enum OperationCode {
        READ        = 10,
        WRITE       = 11,
        LOAD        = 20,
        STORE       = 21,
        ADD         = 30,
        SUBTRACT    = 31,
        DIVIDE      = 32,
        MULTIPLY    = 33,
        MOD         = 34, // (3) nuevo
        EXP         = 35, // (4) nuevo
        BRANCH      = 40,
        BRANCHNEG   = 41,
        BRANCHZERO  = 42,
        HALT        = 43,
        NEWLINE     = 51, // (5) nuevo
        READSTRING  = 52, // (6) nuevo
        WRITESTRING = 53  // (7) nuevo
    };

    Simpletron()
        : memory{},
          accumulator(0.0),
          instructionCounter(0),
          instructionRegister(0),
          operationCode(0),
          operand(0),
          isRunning(true) {}

    // ----------------------------------------------------------------------
    // (1) Fase de carga: intenta cargar automáticamente desde "programa.simp".
    // Si el archivo no existe, recurre a la carga interactiva original.
    // ----------------------------------------------------------------------
    void load() {
        std::cout << "*** Bienvenido al simulador Simpletron v2 ***\n";
        std::cout << "*** Memoria: " << MEMORY_SIZE << " posiciones (000-"
                  << MEMORY_SIZE - 1 << ") ***\n\n";

        if (loadFromFile("programa.simp")) {
            return; // (1a) archivo encontrado y cargado
        }

        std::cout << "*** No se encontro 'programa.simp' en el directorio "
                     "actual. ***\n";
        loadInteractive(); // (1b) respaldo: carga interactiva
    }

    // ----------------------------------------------------------------------
    // Fase de ejecución
    // ----------------------------------------------------------------------
    void execute() {
        std::cout << "*** Inicio de la ejecucion ***\n\n";

        while (isRunning) {
            if (instructionCounter < 0 || instructionCounter >= MEMORY_SIZE) {
                reportError("El contador de instruccion salio de rango de memoria.");
                dumpCore();
                return;
            }

            instructionRegister = static_cast<long long>(
                std::llround(memory[instructionCounter]));
            long long absInstr = std::llabs(instructionRegister);
            operationCode = static_cast<int>(absInstr / 1000); // (2) 3 digitos de operando
            operand = static_cast<int>(absInstr % 1000);

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
                case MOD: // (3)
                    if (!doMod()) return;
                    break;
                case EXP: // (4)
                    if (!doExp()) return;
                    break;
                case BRANCH:
                    instructionCounter = operand;
                    continue;
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
                case NEWLINE: // (5)
                    std::cout << '\n';
                    break;
                case READSTRING: // (6)
                    if (!doReadString()) return;
                    break;
                case WRITESTRING: // (7)
                    if (!doWriteString()) return;
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
    std::array<double, MEMORY_SIZE> memory;
    double accumulator;
    int instructionCounter;
    long long instructionRegister;
    int operationCode;
    int operand;
    bool isRunning;

    // ------------------------------------------------------------------
    // (1) Utilidades de carga
    // ------------------------------------------------------------------
    static std::string trim(const std::string &s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Intenta cargar el programa desde un archivo. Devuelve false si el
    // archivo no existe (para que el llamador use la carga interactiva).
    bool loadFromFile(const std::string &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false; // (1) escenario b: archivo inexistente
        }

        std::cout << "*** Archivo '" << path << "' encontrado. Cargando "
                     "automaticamente... ***\n\n";

        std::string rawLine;
        int address = 0;
        int lineNo = 0;
        int errores = 0;

        while (std::getline(file, rawLine) && address < MEMORY_SIZE) {
            ++lineNo;
            std::string line = trim(rawLine);
            if (line.empty()) continue; // lineas en blanco se ignoran

            try {
                size_t consumed = 0;
                double value = std::stod(line, &consumed);
                if (consumed != line.size()) {
                    throw std::invalid_argument("contenido extra en la linea");
                }

                if (value == -99999) {
                    break; // centinela opcional tambien valido en archivo
                }

                if (std::fabs(value) > MEMORY_MAX_MAGNITUDE) {
                    std::cerr << "*** Linea " << lineNo << " fuera de rango ("
                              << line << "). Se omite. ***\n";
                    ++errores;
                    continue;
                }

                memory[address] = value;
                ++address;
            } catch (const std::exception &) {
                // (1) escenario c: linea invalida -> error controlado, se omite
                std::cerr << "*** Linea " << lineNo << " invalida en '"
                          << path << "': \"" << rawLine << "\" -- se omite. ***\n";
                ++errores;
            }
        }

        std::cout << "\n*** Carga desde archivo completada: " << address
                  << " palabras cargadas";
        if (errores > 0) {
            std::cout << " (" << errores << " linea(s) con error, omitidas)";
        }
        std::cout << ". ***\n\n";
        return true; // (1) escenario a: archivo si existia (se uso, con o sin errores)
    }

    void loadInteractive() {
        std::cout << "*** Ingrese su programa en Simpletron Machine Language "
                     "(SML), ***\n";
        std::cout << "*** una instruccion (o dato) por linea. El operando "
                     "ahora ocupa   ***\n";
        std::cout << "*** 3 digitos, por lo que las instrucciones tienen la "
                     "forma      ***\n";
        std::cout << "*** +OOPPP (OO = codigo de operacion, PPP = direccion "
                     "000-999).  ***\n";
        std::cout << "*** Escriba -99999 para finalizar la introduccion del "
                     "programa. ***\n\n";

        int address = 0;
        double instruction = 0;

        while (address < MEMORY_SIZE) {
            std::cout << std::setw(3) << std::setfill('0') << address << " ? ";

            if (!(std::cin >> instruction)) {
                std::cerr << "*** Entrada invalida. Intente de nuevo. ***\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            if (instruction == -99999) break;

            if (std::fabs(instruction) > MEMORY_MAX_MAGNITUDE) {
                std::cerr << "*** El valor debe estar dentro del rango "
                             "permitido. Intente de nuevo. ***\n";
                continue;
            }

            memory[address] = instruction;
            ++address;
        }

        std::cout << "\n*** El programa se ha cargado en memoria correctamente ***\n\n";
    }

    // ------------------------------------------------------------------
    // Validaciones comunes
    // ------------------------------------------------------------------
    bool checkAddress(int address) {
        if (address < 0 || address >= MEMORY_SIZE) {
            reportError("Direccion de memoria fuera de rango.");
            dumpCore();
            return false;
        }
        return true;
    }

    bool checkOverflow(double value) {
        if (std::isnan(value) || std::isinf(value) ||
            std::fabs(value) > MEMORY_MAX_MAGNITUDE) {
            reportError("Desbordamiento del acumulador (overflow).");
            dumpCore();
            return false;
        }
        return true;
    }

    // ------------------------------------------------------------------
    // Instrucciones basicas (adaptadas a double)
    // ------------------------------------------------------------------
    bool doRead() {
        if (!checkAddress(operand)) return false;
        double value;
        std::cout << "Introduzca un numero (entero o decimal, direccion "
                  << std::setw(3) << std::setfill('0') << operand << ") -> ";
        while (!(std::cin >> value) || std::fabs(value) > MEMORY_MAX_MAGNITUDE) {
            std::cerr << "*** Entrada invalida. Intente de nuevo. ***\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Introduzca un numero (direccion " << std::setw(3)
                      << std::setfill('0') << operand << ") -> ";
        }
        memory[operand] = value;
        return true;
    }

    bool doWrite() {
        if (!checkAddress(operand)) return false;
        std::cout << "Contenido de la direccion " << std::setw(3)
                  << std::setfill('0') << operand << " -> "
                  << formatPlain(memory[operand]) << '\n';
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
        double result = accumulator + memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    bool doSubtract() {
        if (!checkAddress(operand)) return false;
        double result = accumulator - memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    bool doMultiply() {
        if (!checkAddress(operand)) return false;
        double result = accumulator * memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    bool doDivide() {
        if (!checkAddress(operand)) return false;
        if (memory[operand] == 0.0) {
            reportError("Intento de division entre cero.");
            dumpCore();
            return false;
        }
        double result = accumulator / memory[operand];
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    // ------------------------------------------------------------------
    // (3) MOD - residuo/modulo
    // ------------------------------------------------------------------
    bool doMod() {
        if (!checkAddress(operand)) return false;
        if (memory[operand] == 0.0) {
            reportError("Intento de calcular el residuo entre cero.");
            dumpCore();
            return false;
        }
        double result = std::fmod(accumulator, memory[operand]);
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    // ------------------------------------------------------------------
    // (4) EXP - exponenciacion (acumulador = acumulador ^ memoria[operando])
    // ------------------------------------------------------------------
    bool doExp() {
        if (!checkAddress(operand)) return false;
        double base = accumulator;
        double exponent = memory[operand];

        // Caso especial documentado: 0^0 se define como 1 (convencion usual).
        double result = (base == 0.0 && exponent == 0.0) ? 1.0
                                                           : std::pow(base, exponent);
        if (!checkOverflow(result)) return false;
        accumulator = result;
        return true;
    }

    // ------------------------------------------------------------------
    // (6) READSTRING - lee una cadena y la almacena a partir de 'operand'.
    //     memory[operand]      = longitud de la cadena
    //     memory[operand+1..N] = codigo ASCII de cada caracter
    // ------------------------------------------------------------------
    bool doReadString() {
        if (!checkAddress(operand)) return false;

        // Si la entrada anterior fue un numero (>>), queda un '\n' pendiente
        // en el buffer; lo descartamos. Si READSTRING es la primera entrada,
        // no hay nada que descartar y el texto del usuario queda intacto.
        if (std::cin.peek() == '\n') {
            std::cin.get();
        }
        std::cout << "Introduzca una cadena (direccion base " << std::setw(3)
                  << std::setfill('0') << operand << ") -> ";
        std::string text;
        std::getline(std::cin, text);

        int length = static_cast<int>(text.size());
        if (operand + 1 + length > MEMORY_SIZE) {
            reportError("La cadena no cabe en la memoria disponible a partir "
                        "de esa direccion.");
            dumpCore();
            return false;
        }

        memory[operand] = length;
        for (int i = 0; i < length; ++i) {
            memory[operand + 1 + i] =
                static_cast<double>(static_cast<unsigned char>(text[i]));
        }
        return true;
    }

    // ------------------------------------------------------------------
    // (7) WRITESTRING - escribe la cadena almacenada a partir de 'operand'
    //     usando el mismo formato que READSTRING.
    // ------------------------------------------------------------------
    bool doWriteString() {
        if (!checkAddress(operand)) return false;

        long long length = std::llround(memory[operand]);
        if (length < 0 || operand + 1 + length > MEMORY_SIZE) {
            reportError("Cadena invalida o fuera de rango de memoria.");
            dumpCore();
            return false;
        }

        for (long long i = 0; i < length; ++i) {
            int code = static_cast<int>(std::llround(memory[operand + 1 + i]));
            if (code < 0 || code > 255) {
                reportError("Codigo ASCII invalido dentro de la cadena.");
                dumpCore();
                return false;
            }
            std::cout << static_cast<char>(code);
        }
        return true;
    }

    void reportError(const std::string &message) {
        std::cerr << "\n*** Error en tiempo de ejecucion: " << message << " ***\n";
        std::cerr << "*** Simpletron finalizando la ejecucion. ***\n\n";
    }

    // ------------------------------------------------------------------
    // Formateo de valores
    // ------------------------------------------------------------------

    // Version "amigable" usada por WRITE (sin relleno de ceros).
    static std::string formatPlain(double value) {
        std::ostringstream oss;
        double rounded = std::round(value * 100.0) / 100.0;
        if (std::fabs(rounded - std::round(rounded)) < 1e-9) {
            oss << static_cast<long long>(std::round(rounded));
        } else {
            oss << std::fixed << std::setprecision(2) << rounded;
        }
        return oss.str();
    }

    // Version fija usada por el volcado de memoria (dump), con signo y
    // ancho constante para que la tabla quede alineada.
    static std::string formatWord(double value) {
        std::string sign = (value < 0) ? "-" : "+";
        double absValue = std::fabs(value);
        bool isIntegral = std::fabs(absValue - std::round(absValue)) < 1e-9;

        std::ostringstream oss;
        if (isIntegral) {
            long long intPart = static_cast<long long>(std::round(absValue));
            oss << std::setw(6) << std::setfill('0') << intPart;
        } else {
            long long intPart = static_cast<long long>(absValue);
            int fracDigits =
                static_cast<int>(std::round((absValue - intPart) * 100.0));
            oss << std::setw(6) << std::setfill('0') << intPart << "."
                << std::setw(2) << std::setfill('0') << fracDigits;
        }
        return sign + oss.str();
    }

    // ------------------------------------------------------------------
    // (2) Volcado de memoria (core dump) ajustado a 1000 posiciones,
    // mostrado en bloques de 10 columnas x 100 filas.
    // ------------------------------------------------------------------
    void dumpCore() {
        std::cout << std::setfill(' ');
        std::cout << "\nREGISTROS:\n";
        std::cout << "Acumulador                 " << formatWord(accumulator) << '\n';
        std::cout << "contador de instruccion         "
                  << std::setw(3) << std::setfill('0') << instructionCounter << '\n';
        std::cout << std::setfill(' ');
        std::cout << "registro de instruccion       " << instructionRegister << '\n';
        std::cout << "codigo de operacion               "
                  << std::setw(2) << std::setfill('0') << operationCode << '\n';
        std::cout << std::setfill(' ');
        std::cout << "operando                        "
                  << std::setw(3) << std::setfill('0') << operand << "\n\n";

        std::cout << "MEMORIA (1000 posiciones, 10 columnas x 100 filas):\n";
        std::cout << "      ";
        for (int col = 0; col < 10; ++col) {
            std::cout << std::setfill(' ') << std::setw(11) << col;
        }
        std::cout << '\n';

        for (int row = 0; row < MEMORY_SIZE / 10; ++row) {
            std::cout << std::setfill('0') << std::setw(3) << row * 10
                      << std::setfill(' ') << " ";
            for (int col = 0; col < 10; ++col) {
                std::cout << std::setw(11) << formatWord(memory[row * 10 + col]);
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

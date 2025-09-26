#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>  // Para ocultar contraseña en Windows
#else
#include <termios.h>
#include <unistd.h>
#endif

// =============================
// Configuración general
// =============================

#define MAX_MEDICAMENTOS 100
#define MAX_VENTAS 1000

// ⚠️ Contraseña del dueño (solo referencia, no se muestra en consola)
// Contraseña: admin123
const char OWNER_PASS[] = "admin123";

// =============================
// Estructuras en memoria
// =============================

typedef struct {
    int codigo;
    char nombre[50];
    float precio;
    int stock;
    int stockCritico;
    int ventaLibre; // 1 = libre, 0 = bajo receta
} Medicamento;

typedef struct {
    char fecha[11];   // dd/mm/yyyy
    int codigo;
    int cantidad;
    float total;
    char dni[15];     // solo para bajo receta
} Venta;

// =============================
// Variables globales
// =============================

Medicamento medicamentos[MAX_MEDICAMENTOS];
int totalMedicamentos = 0;

Venta ventas[MAX_VENTAS];
int totalVentas = 0;

// =============================
// Funciones auxiliares
// =============================

// Ocultar entrada de contraseña
void leerPassword(char *pass, int maxLen) {
    int i = 0;
    char c;
#ifdef _WIN32
    while ((c = _getch()) != '\r' && i < maxLen - 1) {
        if (c == '\b' && i > 0) { i--; printf("\b \b"); }
        else if (c != '\b') { pass[i++] = c; printf("*"); }
    }
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while ((c = getchar()) != '\n' && i < maxLen - 1) {
        if (c == 127 && i > 0) { i--; }
        else { pass[i++] = c; }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    pass[i] = '\0';
    printf("\n");
}

// Obtener fecha actual
void obtenerFecha(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%02d/%02d/%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

// Buscar medicamento por código
int buscarMedicamento(int codigo) {
    for (int i = 0; i < totalMedicamentos; i++) {
        if (medicamentos[i].codigo == codigo) return i;
    }
    return -1;
}

// =============================
// Módulo de medicamentos
// =============================

void agregarMedicamento() {
    if (totalMedicamentos >= MAX_MEDICAMENTOS) {
        printf("Error: capacidad máxima alcanzada.\n");
        return;
    }

    Medicamento m;
    printf("Codigo: "); scanf("%d", &m.codigo);
    if (buscarMedicamento(m.codigo) != -1) {
        printf("Ya existe un medicamento con ese codigo.\n");
        return;
    }
    printf("Nombre: "); scanf(" %[^\n]", m.nombre);
    printf("Precio: "); scanf("%f", &m.precio);
    printf("Stock inicial: "); scanf("%d", &m.stock);
    printf("Stock critico: "); scanf("%d", &m.stockCritico);
    printf("Venta libre? (1=si,0=no): "); scanf("%d", &m.ventaLibre);

    medicamentos[totalMedicamentos++] = m;
    printf("Medicamento agregado correctamente.\n");
}

void editarMedicamento() {
    int codigo;
    printf("Ingrese codigo del medicamento a editar: ");
    scanf("%d", &codigo);

    int idx = buscarMedicamento(codigo);
    if (idx == -1) {
        printf("Medicamento no encontrado.\n");
        return;
    }

    printf("Editar nombre (actual: %s): ", medicamentos[idx].nombre);
    scanf(" %[^\n]", medicamentos[idx].nombre);
    printf("Editar precio (actual: %.2f): ", medicamentos[idx].precio);
    scanf("%f", &medicamentos[idx].precio);
    printf("Editar stock (actual: %d): ", medicamentos[idx].stock);
    scanf("%d", &medicamentos[idx].stock);
    printf("Editar stock critico (actual: %d): ", medicamentos[idx].stockCritico);
    scanf("%d", &medicamentos[idx].stockCritico);
    printf("Editar venta libre (1=si,0=no, actual: %d): ", medicamentos[idx].ventaLibre);
    scanf("%d", &medicamentos[idx].ventaLibre);

    printf("Medicamento actualizado correctamente.\n");
}

void eliminarMedicamento() {
    int codigo;
    printf("Ingrese codigo del medicamento a eliminar: ");
    scanf("%d", &codigo);

    int idx = buscarMedicamento(codigo);
    if (idx == -1) {
        printf("Medicamento no encontrado.\n");
        return;
    }

    for (int i = idx; i < totalMedicamentos - 1; i++) {
        medicamentos[i] = medicamentos[i + 1];
    }
    totalMedicamentos--;
    printf("Medicamento eliminado correctamente.\n");
}

void exportarMedicamentosCSV() {
    FILE *f = fopen("medicamentos.csv", "w");
    if (!f) {
        printf("Error al abrir archivo CSV.\n");
        return;
    }

    fprintf(f, "Codigo,Nombre,Precio,Stock,StockCritico,VentaLibre\n");
    for (int i = 0; i < totalMedicamentos; i++) {
        fprintf(f, "%d,%s,%.2f,%d,%d,%d\n",
                medicamentos[i].codigo,
                medicamentos[i].nombre,
                medicamentos[i].precio,
                medicamentos[i].stock,
                medicamentos[i].stockCritico,
                medicamentos[i].ventaLibre);
    }
    fclose(f);
    printf("Medicamentos exportados a medicamentos.csv\n");
}

// =============================
// Menú principal
// =============================

int verificarDuenio() {
    char pass[50];
    printf("Ingrese contrasena de duenio: ");
    leerPassword(pass, 50);
    return strcmp(pass, OWNER_PASS) == 0;
}

void menuPrincipal() {
    int opcion;
    do {
        printf("\n=== Sistema de Farmacia ===\n");
        printf("1. Agregar medicamento\n");
        printf("2. Editar medicamento\n");
        printf("3. Eliminar medicamento\n");
        printf("4. Exportar medicamentos a CSV\n");
        printf("0. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: agregarMedicamento(); break;
            case 2: if (verificarDuenio()) editarMedicamento();
                    else printf("Acceso denegado.\n"); break;
            case 3: if (verificarDuenio()) eliminarMedicamento();
                    else printf("Acceso denegado.\n"); break;
            case 4: exportarMedicamentosCSV(); break;
            case 0: printf("Saliendo...\n"); break;
            default: printf("Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// =============================
// Programa principal
// =============================

int main() {
    menuPrincipal();
    return 0;
}

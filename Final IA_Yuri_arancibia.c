#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEDICAMENTOS 100
#define MAX_VENTAS 1000
#define MAX_NOMBRE 50
#define MAX_DIAS 31
#define DUENO_PASSWORD "1234"  // contrasena inicial del dueno

// =============================
// Estructuras
// =============================

typedef struct {
    int codigo;
    char nombre[MAX_NOMBRE];
    float precio;
    int stock;
    int stockCritico;
    int ventaLibre; // 1 = libre, 0 = bajo receta
} Medicamento;

typedef struct {
    int dia;
    int codigoMedicamento;
    int cantidad;
    float total;
    char dniCliente[15]; // solo si es bajo receta
} Venta;

// =============================
// Variables globales
// =============================

Medicamento medicamentos[MAX_MEDICAMENTOS];
int totalMedicamentos = 0;

Venta ventas[MAX_VENTAS];
int totalVentas = 0;

char duenoPassword[20] = DUENO_PASSWORD;

// =============================
// Funciones utilitarias
// =============================

int verificarDueno() {
    char pass[20];
    printf("Ingrese contrasena del dueno: ");
    scanf("%s", pass);
    if (strcmp(pass, duenoPassword) == 0) return 1;
    return 0;
}

int buscarMedicamentoPorCodigo(int codigo) {
    for (int i = 0; i < totalMedicamentos; i++) {
        if (medicamentos[i].codigo == codigo) return i;
    }
    return -1;
}

// =============================
// Gestion de medicamentos
// =============================

void agregarMedicamento() {
    if (totalMedicamentos >= MAX_MEDICAMENTOS) {
        printf("No se pueden agregar mas medicamentos.\n");
        return;
    }
    Medicamento m;
    printf("Codigo: ");
    scanf("%d", &m.codigo);
    if (buscarMedicamentoPorCodigo(m.codigo) != -1) {
        printf("El codigo ya existe.\n");
        return;
    }
    printf("Nombre: ");
    scanf(" %[^\n]", m.nombre);
    printf("Precio: ");
    scanf("%f", &m.precio);
    printf("Cantidad en stock: ");
    scanf("%d", &m.stock);
    printf("Stock critico: ");
    scanf("%d", &m.stockCritico);
    printf("Es de venta libre? (1=Si, 0=No): ");
    scanf("%d", &m.ventaLibre);

    medicamentos[totalMedicamentos++] = m;
    printf("Medicamento agregado con exito.\n");
}

void editarMedicamento() {
    int codigo;
    printf("Ingrese codigo de medicamento a editar: ");
    scanf("%d", &codigo);
    int idx = buscarMedicamentoPorCodigo(codigo);
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
    printf("Editar venta libre (actual: %d): ", medicamentos[idx].ventaLibre);
    scanf("%d", &medicamentos[idx].ventaLibre);
    printf("Medicamento editado con exito.\n");
}

void eliminarMedicamento() {
    int codigo;
    printf("Ingrese codigo de medicamento a eliminar: ");
    scanf("%d", &codigo);
    int idx = buscarMedicamentoPorCodigo(codigo);
    if (idx == -1) {
        printf("Medicamento no encontrado.\n");
        return;
    }
    for (int i = idx; i < totalMedicamentos - 1; i++) {
        medicamentos[i] = medicamentos[i + 1];
    }
    totalMedicamentos--;
    printf("Medicamento eliminado.\n");
}

void exportarMedicamentosCSV() {
    FILE *f = fopen("medicamentos.csv", "w");
    if (!f) {
        printf("Error al abrir el archivo.\n");
        return;
    }
    fprintf(f, "Codigo,Nombre,Precio,Stock,StockCritico,VentaLibre\n");
    for (int i = 0; i < totalMedicamentos; i++) {
        fprintf(f, "%d,%s,%.2f,%d,%d,%d\n",
                medicamentos[i].codigo, medicamentos[i].nombre,
                medicamentos[i].precio, medicamentos[i].stock,
                medicamentos[i].stockCritico, medicamentos[i].ventaLibre);
    }
    fclose(f);
    printf("Medicamentos exportados a medicamentos.csv\n");
}

// =============================
// Gestion de ventas
// =============================

void venderMedicamento() {
    if (totalVentas >= MAX_VENTAS) {
        printf("No se pueden registrar mas ventas.\n");
        return;
    }
    int codigo, cantidad, dia;
    printf("Codigo de medicamento: ");
    scanf("%d", &codigo);
    int idx = buscarMedicamentoPorCodigo(codigo);
    if (idx == -1) {
        printf("Medicamento no encontrado.\n");
        return;
    }
    printf("Cantidad: ");
    scanf("%d", &cantidad);
    if (cantidad > medicamentos[idx].stock) {
        printf("No hay suficiente stock.\n");
        return;
    }
    printf("Dia de venta (1-31): ");
    scanf("%d", &dia);

    Venta v;
    v.dia = dia;
    v.codigoMedicamento = codigo;
    v.cantidad = cantidad;
    v.total = cantidad * medicamentos[idx].precio;
    if (!medicamentos[idx].ventaLibre) {
        printf("Medicamento bajo receta, ingrese DNI del cliente: ");
        scanf("%s", v.dniCliente);
    } else {
        strcpy(v.dniCliente, "-");
    }

    ventas[totalVentas++] = v;
    medicamentos[idx].stock -= cantidad;
    printf("Venta registrada.\n");
}

// =============================
// Informes
// =============================

void informeDiario() {
    int dia;
    printf("Ingrese dia: ");
    scanf("%d", &dia);
    float total = 0;
    int cant = 0;
    printf("Informe del dia %d:\n", dia);
    for (int i = 0; i < totalVentas; i++) {
        if (ventas[i].dia == dia) {
            printf("Cod %d, Cant %d, Total %.2f\n",
                   ventas[i].codigoMedicamento,
                   ventas[i].cantidad, ventas[i].total);
            total += ventas[i].total;
            cant++;
        }
    }
    printf("Total del dia: %.2f en %d ventas\n", total, cant);
}

void informeMensual() {
    float total = 0;
    int cant = 0;
    printf("Informe mensual:\n");
    for (int i = 0; i < totalVentas; i++) {
        total += ventas[i].total;
        cant++;
    }
    printf("Total mensual: %.2f en %d ventas\n", total, cant);
}

void reiniciarMes() {
    totalVentas = 0;
    printf("Datos de ventas reiniciados.\n");
}

void cambiarPassword() {
    char nueva[20];
    printf("Ingrese nueva contrasena: ");
    scanf("%s", nueva);
    strcpy(duenoPassword, nueva);
    printf("Contrasena cambiada con exito.\n");
}

// =============================
// Menu principal
// =============================

void menu() {
    int opcion;
    do {
        printf("\n=== Sistema de Farmacia ===\n");
        printf("1. Agregar medicamento\n");
        printf("2. Editar medicamento (dueno)\n");
        printf("3. Eliminar medicamento (dueno)\n");
        printf("4. Exportar medicamentos a CSV\n");
        printf("5. Vender medicamento\n");
        printf("6. Informe diario (dueno)\n");
        printf("7. Informe mensual (dueno)\n");
        printf("8. Reiniciar mes (dueno)\n");
        printf("9. Cambiar contrasena (dueno)\n");
        printf("0. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: agregarMedicamento(); break;
            case 2: 
                if (verificarDueno()) {
                    editarMedicamento();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 3: 
                if (verificarDueno()) {
                    eliminarMedicamento();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 4: exportarMedicamentosCSV(); break;
            case 5: venderMedicamento(); break;
            case 6: 
                if (verificarDueno()) {
                    informeDiario();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 7: 
                if (verificarDueno()) {
                    informeMensual();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 8: 
                if (verificarDueno()) {
                    reiniciarMes();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 9: 
                if (verificarDueno()) {
                    cambiarPassword();
                } else {
                    printf("Acceso denegado.\n");
                }
                break;
            case 0: printf("Saliendo...\n"); break;
            default: printf("Opcion invalida.\n");
        }
    } while (opcion != 0);
}

// =============================
// Programa principal
// =============================

int main() {
    menu();
    return 0;
}

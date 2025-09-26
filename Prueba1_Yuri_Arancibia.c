/* farmacia_no_files.c
   Sistema de administración de farmacia (sin archivos, sin structs)
   Compilar: gcc -std=c11 -O2 -Wall farmacia_no_files.c -o farmacia
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_MEDICINES 200
#define MAX_NAME_LEN 64
#define MAX_SALES 5000
#define MAX_RX_RECORDS 3000
#define MAX_INPUT 128
#define DAYS_IN_MONTH 31

/* ---------------------------
   Datos: arrays paralelos
   --------------------------- */
static int med_count = 0;
static int med_code[MAX_MEDICINES];
static char med_name[MAX_MEDICINES][MAX_NAME_LEN];
static double med_price[MAX_MEDICINES];
static int med_stock[MAX_MEDICINES];
static int med_is_otc[MAX_MEDICINES];      /* 1 = venta libre, 0 = bajo receta */
static int med_critical[MAX_MEDICINES];    /* stock crítico por medicamento */

/* Ventas (registro de ventas individuales para el mes en curso) */
static int sale_count = 0;
static int sale_day[MAX_SALES];         /* día 1..31 */
static double sale_amount[MAX_SALES];   /* importe en pesos */
static int sale_qty[MAX_SALES];         /* cantidad de ítems vendidos en la operación */
static int sale_med_code[MAX_SALES];    /* código de medicamento vendido */

/* Registros de ventas bajo receta (por requerimiento legal) */
static int rx_count = 0;
static char rx_med_name[MAX_RX_RECORDS][MAX_NAME_LEN];
static int rx_qty[MAX_RX_RECORDS];
static char rx_dni[MAX_RX_RECORDS][32];
static int rx_day[MAX_RX_RECORDS];

/* Contraseña del dueño (en memoria). Por simplicidad la fijamos aquí; puede cambiarse al inicio. */
static char owner_password[64] = "admin123"; /* se puede cambiar en ejecución */

/* ---------------------------
   Utilidades de entrada
   --------------------------- */
static void read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* lee entero con prompt, con validación; devuelve 1 si OK, 0 si cancelado (input vacío) */
static int prompt_int(const char *prompt, int *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (buf[0] == '\0') return 0; /* cancel */
        char *end;
        long val = strtol(buf, &end, 10);
        if (end == buf || *end != '\0') {
            printf("Entrada inválida. Ingrese un número entero.\n");
            continue;
        }
        *out_val = (int)val;
        return 1;
    }
}

/* lee double */
static int prompt_double(const char *prompt, double *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (buf[0] == '\0') return 0;
        char *end;
        double val = strtod(buf, &end);
        if (end == buf || *end != '\0') {
            printf("Entrada inválida. Ingrese un número (puede tener decimales).\n");
            continue;
        }
        *out_val = val;
        return 1;
    }
}

/* trim espacios */
static void trim(char *s) {
    /* trim leading */
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    /* trim trailing */
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) { s[len-1] = '\0'; len--; }
}

/* ---------------------------
   Búsquedas y validaciones
   --------------------------- */
static int find_med_index_by_code(int code) {
    for (int i = 0; i < med_count; ++i) if (med_code[i] == code) return i;
    return -1;
}

/* ---------------------------
   Funciones del sistema
   --------------------------- */

static void show_header(void) {
    printf("=========================================\n");
    printf("Sistema de Farmacia (memoria volátil) \n");
    printf("ATENCIÓN: NO usa archivos. Todo se pierde si se corta la luz.\n");
    printf("=========================================\n");
}

/* Añadir nuevo medicamento */
static void add_medicine(void) {
    if (med_count >= MAX_MEDICINES) {
        printf("No se pueden agregar más medicamentos (límite alcanzado).\n");
        return;
    }

    int code;
    if (!prompt_int("Ingrese código numérico del medicamento (vacío para cancelar): ", &code)) return;
    if (find_med_index_by_code(code) != -1) { printf("Código ya existente.\n"); return; }

    char name[MAX_NAME_LEN];
    printf("Nombre del medicamento: ");
    read_line(name, sizeof(name));
    trim(name);
    if (name[0] == '\0') { printf("Nombre vacío. Cancelado.\n"); return; }

    double price;
    if (!prompt_double("Precio (ej: 123.45): ", &price)) return;
    if (price < 0) { printf("Precio inválido.\n"); return; }

    int stock;
    if (!prompt_int("Cantidad en stock (entero): ", &stock)) return;
    if (stock < 0) { printf("Stock inválido.\n"); return; }

    char otcbuf[MAX_INPUT];
    int is_otc = -1;
    while (is_otc == -1) {
        printf("Es venta libre? (s/n): ");
        read_line(otcbuf, sizeof(otcbuf));
        if (otcbuf[0] == '\0') return;
        if (tolower((unsigned char)otcbuf[0]) == 's') is_otc = 1;
        else if (tolower((unsigned char)otcbuf[0]) == 'n') is_otc = 0;
        else printf("Respuesta inválida, use s o n.\n");
    }

    int crit;
    if (!prompt_int("Stock crítico (entero): ", &crit)) return;
    if (crit < 0) { printf("Stock crítico inválido.\n"); return; }

    int idx = med_count++;
    med_code[idx] = code;
    strncpy(med_name[idx], name, MAX_NAME_LEN-1); med_name[idx][MAX_NAME_LEN-1] = '\0';
    med_price[idx] = price;
    med_stock[idx] = stock;
    med_is_otc[idx] = is_otc;
    med_critical[idx] = crit;

    printf("Medicamento agregado correctamente (índice %d).\n", idx);
}

/* Listar todos los medicamentos */
static void list_medicines(void) {
    if (med_count == 0) { printf("No hay medicamentos registrados.\n"); return; }
    printf("Listado de medicamentos:\n");
    printf("Código | Nombre                          | Precio    | Stock | Tipo | Crítico\n");
    printf("-----------------------------------------------------------------------------\n");
    for (int i = 0; i < med_count; ++i) {
        printf("%6d | %-30s | %8.2f | %5d | %4s | %7d\n",
               med_code[i],
               med_name[i],
               med_price[i],
               med_stock[i],
               med_is_otc[i] ? "OTC" : "RX",
               med_critical[i]);
    }
}

/* Buscar y mostrar un medicamento por código */
static void show_medicine_by_code(void) {
    int code;
    if (!prompt_int("Ingrese código de medicamento (vacío para cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
    if (idx == -1) { printf("No encontrado.\n"); return; }
    printf("Código: %d\nNombre: %s\nPrecio: %.2f\nStock: %d\nTipo: %s\nStock crítico: %d\n",
           med_code[idx], med_name[idx], med_price[idx], med_stock[idx],
           med_is_otc[idx] ? "Venta libre (OTC)" : "Bajo receta (RX)", med_critical[idx]);
}

/* Vender medicamento (flujo general que delega en OTC o RX) */
static void sell_medicine(void) {
    if (sale_count >= MAX_SALES) { printf("No se pueden registrar más ventas este mes (límite alcanzado).\n"); return; }
    int code;
    if (!prompt_int("Código de medicamento a vender (vacío para cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
    if (idx == -1) { printf("Código no existe.\n"); return; }

    printf("Medicamento: %s | Precio unitario: %.2f | Stock actual: %d | Tipo: %s\n",
           med_name[idx], med_price[idx], med_stock[idx], med_is_otc[idx] ? "OTC" : "RX");

    int qty;
    if (!prompt_int("Cantidad a vender: ", &qty)) return;
    if (qty <= 0) { printf("Cantidad debe ser mayor que 0.\n"); return; }
    if (qty > med_stock[idx]) { printf("Stock insuficiente.\n"); return; }

    int day;
    if (!prompt_int("Día de la venta (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Día inválido.\n"); return; }

    double total = qty * med_price[idx];

    /* Registrar venta general */
    sale_day[sale_count] = day;
    sale_amount[sale_count] = total;
    sale_qty[sale_count] = qty;
    sale_med_code[sale_count] = med_code[idx];
    sale_count++;

    /* Reducir stock */
    med_stock[idx] -= qty;

    /* Si es con receta, registrar extra (nombre, cantidad, DNI) */
    if (!med_is_otc[idx]) {
        if (rx_count >= MAX_RX_RECORDS) {
            printf("Límite de registros RX alcanzado. NO se pudo guardar registro requerido por ley.\n");
        } else {
            char dni_buf[32];
            printf("Ingrese DNI del comprador (vacío para cancelar registro RX - ¡ATENCIÓN! esto dejará sin registro legal): ");
            read_line(dni_buf, sizeof(dni_buf));
            trim(dni_buf);
            if (dni_buf[0] == '\0') {
                printf("Registro RX vacío: registro no guardado. (Advertencia legal)\n");
            } else {
                strncpy(rx_med_name[rx_count], med_name[idx], MAX_NAME_LEN-1);
                rx_med_name[rx_count][MAX_NAME_LEN-1] = '\0';
                rx_qty[rx_count] = qty;
                strncpy(rx_dni[rx_count], dni_buf, sizeof(rx_dni[rx_count])-1);
                rx_dni[rx_count][sizeof(rx_dni[rx_count])-1] = '\0';
                rx_day[rx_count] = day;
                rx_count++;
            }
        }
    }

    printf("Venta registrada: $%.2f | Día %d | Quedan %d unidades en stock.\n", total, day, med_stock[idx]);
}

/* Informe de ventas por día y total mensual (se muestra por pantalla) */
static void report_monthly(void) {
    if (sale_count == 0) { printf("No hay ventas registradas este mes.\n"); return; }

    double total_month = 0.0;
    int sales_per_day[DAYS_IN_MONTH+1];
    for (int i = 0; i <= DAYS_IN_MONTH; ++i) sales_per_day[i] = 0;

    for (int i = 0; i < sale_count; ++i) {
        int d = sale_day[i];
        if (d >=1 && d <= DAYS_IN_MONTH) sales_per_day[d] += 1;
        total_month += sale_amount[i];
    }

    printf("===== Informe mensual (en memoria) =====\n");
    printf("Total ventas (importe): $%.2f\n", total_month);
    printf("Ventas por día (día: cantidad de ventas):\n");
    for (int d = 1; d <= DAYS_IN_MONTH; ++d) {
        if (sales_per_day[d] > 0) printf("Día %2d: %d\n", d, sales_per_day[d]);
    }
    printf("Total operaciones registradas en el mes: %d\n", sale_count);
    printf("===== Fin informe =====\n");
    printf("(Advertencia: este informe NO se guarda en disco. Si desea exportar en el futuro, habilite la opción de archivos.)\n");
}

/* Informe de ventas de un día específico */
static void report_day(void) {
    int day;
    if (!prompt_int("Ingrese día a consultar (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Día inválido.\n"); return; }

    double total_day = 0.0;
    int count_day = 0;
    for (int i = 0; i < sale_count; ++i) {
        if (sale_day[i] == day) { total_day += sale_amount[i]; count_day++; }
    }

    printf("Informe día %d: %d operaciones | Total importe: $%.2f\n", day, count_day, total_day);
}

/* Mostrar stock crítico (solo dueño) */
static void report_stock_critical(void) {
    int found = 0;
    printf("Medicamentos por debajo del stock crítico:\n");
    for (int i = 0; i < med_count; ++i) {
        if (med_stock[i] <= med_critical[i]) {
            found = 1;
            printf("Código %d | %s | Stock: %d | Crítico: %d\n",
                   med_code[i], med_name[i], med_stock[i], med_critical[i]);
        }
    }
    if (!found) printf("Ningún medicamento está por debajo (o en) su stock crítico.\n");
}

/* Mostrar registros RX (por ley) */
static void show_rx_records(void) {
    if (rx_count == 0) { printf("No hay registros de ventas bajo receta.\n"); return; }
    printf("Registros de ventas bajo receta (RX):\n");
    printf("Día | Medicamento                     | Cant | DNI\n");
    printf("-------------------------------------------------\n");
    for (int i = 0; i < rx_count; ++i) {
        printf("%3d | %-30s | %4d | %s\n", rx_day[i], rx_med_name[i], rx_qty[i], rx_dni[i]);
    }
}

/* Reset mensual: borra ventas y registros RX (solo dueño) */
static void reset_month(void) {
    sale_count = 0;
    rx_count = 0;
    printf("Datos de ventas del mes borrados. El catálogo de medicamentos NO fue afectado.\n");
}

/* Autenticación simple del dueño */
static int authenticate_owner(void) {
    char buf[64];
    printf("Ingrese contraseña del dueño (vacío para cancelar): ");
    read_line(buf, sizeof(buf));
    if (buf[0] == '\0') return 0;
    if (strcmp(buf, owner_password) == 0) return 1;
    printf("Contraseña incorrecta.\n");
    return 0;
}

/* Cambiar contraseña del dueño (requiere la actual) */
static void change_owner_password(void) {
    printf("Cambio de contraseña del dueño.\n");
    char current[64];
    printf("Ingrese contraseña actual: ");
    read_line(current, sizeof(current));
    if (strcmp(current, owner_password) != 0) {
        printf("Contraseña actual incorrecta.\n");
        return;
    }
    char newp[64], conf[64];
    printf("Nueva contraseña: ");
    read_line(newp, sizeof(newp));
    if (newp[0] == '\0') { printf("Contraseña vacía. Cancelado.\n"); return; }
    printf("Confirme nueva contraseña: ");
    read_line(conf, sizeof(conf));
    if (strcmp(newp, conf) != 0) { printf("No coinciden. Cancelado.\n"); return; }
    strncpy(owner_password, newp, sizeof(owner_password)-1);
    owner_password[sizeof(owner_password)-1] = '\0';
    printf("Contraseña cambiada.\n");
}

/* Menú principal */
static void main_menu(void) {
    int running = 1;
    while (running) {
        show_header();
        printf("Opciones:\n");
        printf(" 1) Agregar medicamento\n");
        printf(" 2) Listar medicamentos\n");
        printf(" 3) Mostrar medicamento por código\n");
        printf(" 4) Vender medicamento\n");
        printf(" 5) Informe mensual (pantalla)\n");
        printf(" 6) Informe de un día\n");
        printf(" 7) Mostrar registros RX (solo dueño)\n");
        printf(" 8) Reporte stock crítico (solo dueño)\n");
        printf(" 9) Reset mes (solo dueño)\n");
        printf("10) Cambiar contraseña del dueño\n");
        printf(" 0) Salir\n");
        printf("---------------------------------\n");

        int option;
        if (!prompt_int("Seleccione opción: ", &option)) { printf("Entrada vacía. Volviendo al menú.\n"); continue; }

        switch (option) {
            case 1: add_medicine(); break;
            case 2: list_medicines(); break;
            case 3: show_medicine_by_code(); break;
            case 4: sell_medicine(); break;
            case 5: report_monthly(); break;
            case 6: report_day(); break;
            case 7:
                if (authenticate_owner()) show_rx_records();
                else printf("No autorizado.\n");
                break;
            case 8:
                if (authenticate_owner()) report_stock_critical();
                else printf("No autorizado.\n");
                break;
            case 9:
                if (authenticate_owner()) {
                    printf("CONFIRME: desea borrar TODOS los datos de ventas del mes actual? (s/n): ");
                    char c[8]; read_line(c, sizeof(c));
                    if (tolower((unsigned char)c[0]) == 's') reset_month();
                    else printf("Operación cancelada.\n");
                } else printf("No autorizado.\n");
                break;
            case 10: change_owner_password(); break;
            case 0: running = 0; break;
            default: printf("Opción inválida.\n"); break;
        }

        printf("\nPresione ENTER para continuar...");
        char tmp[8]; read_line(tmp, sizeof(tmp));
    }
}

/* ---------------------------
   Programa principal
   --------------------------- */
int main(void) {
    setbuf(stdout, NULL); /* para evitar buffering en algunos entornos */
    printf("Inicializando sistema de farmacia (modo sin archivos)...\n");
    printf("Contraseña inicial del dueño: %s (recomiende cambiarla)\n", owner_password);
    main_menu();
    printf("Saliendo. Todos los datos en memoria serán perdidos.\n");
    return 0;
}

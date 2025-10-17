/* farmacia_no_globals.c
   Versión completa sin variables globales, sin structs/typedef, sin archivos.
   - Máx medicamentos: 200
   - Máx ventas por mes: 5000
   - Exportar CSV -> imprime en pantalla (no escribe archivos)
   - Compilar: gcc -std=c11 -O2 -Wall farmacia_no_globals.c -o farmacia
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

#define MAX_MEDICINES 200
#define MAX_NAME_LEN 64
#define MAX_SALES 5000
#define MAX_INPUT 128
#define DAYS_IN_MONTH 31

/* ----------------- Utilidades ----------------- */

void read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len && buf[len-1] == '\n') buf[len-1] = '\0';
}

void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) { s[len-1] = '\0'; len--; }
}

/* Leer contraseña oculta (UNIX). En Windows, se lee visible. */
void read_password(char *out, size_t maxlen) {
    size_t i = 0;
#ifdef _WIN32
    /* Windows (simple): no ocultamos, para compatibilidad con entornos */
    read_line(out, maxlen);
    return;
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int c;
    while ((c = getchar()) != '\n' && c != EOF && i + 1 < maxlen) {
        out[i++] = (char)c;
    }
    out[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
#endif
}

/* Leer entero con prompt - retorna 1 si OK, 0 si vacio (cancel) */
int prompt_int(const char *prompt, int *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        trim(buf);
        if (buf[0] == '\0') return 0;
        char *end;
        long v = strtol(buf, &end, 10);
        if (end == buf || *end != '\0') {
            printf("Entrada inválida. Ingrese un número entero.\n");
            continue;
        }
        *out_val = (int)v;
        return 1;
    }
}

/* Leer double con prompt - retorna 1 si OK, 0 si vacio */
int prompt_double(const char *prompt, double *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        trim(buf);
        if (buf[0] == '\0') return 0;
        char *end;
        double v = strtod(buf, &end);
        if (end == buf || *end != '\0') {
            printf("Entrada inválida. Ingrese un número válido.\n");
            continue;
        }
        *out_val = v;
        return 1;
    }
}

/* Buscar índice de medicamento por código (recibe arrays desde main) */
int find_med_index_by_code(int code, int med_code[], int med_count) {
    for (int i = 0; i < med_count; ++i) if (med_code[i] == code) return i;
    return -1;
}

/* ----------------- Módulo Medicamentos ----------------- */

/* Agregar medicamento */
void add_medicine(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int *med_count_ptr
) {
    int med_count = *med_count_ptr;
    if (med_count >= MAX_MEDICINES) { printf("Límite de catálogo alcanzado.\n"); return; }

    int code;
    if (!prompt_int("Ingrese código (vaciar para cancelar): ", &code)) return;
    if (find_med_index_by_code(code, med_code, med_count) != -1) { printf("Código ya existente.\n"); return; }

    char name[MAX_NAME_LEN];
    printf("Nombre: ");
    read_line(name, sizeof(name));
    trim(name);
    if (name[0] == '\0') { printf("Nombre vacío. Cancelado.\n"); return; }

    double price;
    if (!prompt_double("Precio (ej: 123.45): ", &price)) return;
    if (price < 0) { printf("Precio inválido.\n"); return; }

    int stock;
    if (!prompt_int("Stock inicial: ", &stock)) return;
    if (stock < 0) { printf("Stock inválido.\n"); return; }

    char otcbuf[MAX_INPUT];
    int is_otc = -1;
    while (is_otc == -1) {
        printf("Venta libre? (s/n): ");
        read_line(otcbuf, sizeof(otcbuf));
        trim(otcbuf);
        if (otcbuf[0] == '\0') { printf("Cancelado.\n"); return; }
        if (tolower((unsigned char)otcbuf[0]) == 's') is_otc = 1;
        else if (tolower((unsigned char)otcbuf[0]) == 'n') is_otc = 0;
        else printf("Respuesta inválida, ingrese s o n.\n");
    }

    int crit;
    if (!prompt_int("Stock crítico: ", &crit)) return;
    if (crit < 0) { printf("Stock crítico inválido.\n"); return; }

    /* Guardar */
    int idx = med_count;
    med_code[idx] = code;
    strncpy(med_name[idx], name, MAX_NAME_LEN-1); med_name[idx][MAX_NAME_LEN-1] = '\0';
    med_price[idx] = price;
    med_stock[idx] = stock;
    med_is_otc[idx] = is_otc;
    med_critical[idx] = crit;
    med_count++;
    *med_count_ptr = med_count;

    printf("Medicamento agregado (índice %d).\n", idx);
}

/* Listar medicamentos */
void list_medicines(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int med_count
) {
    if (med_count == 0) { printf("No hay medicamentos registrados.\n"); return; }
    printf("Código | Nombre                           | Precio   | Stock | Tipo | Crítico\n");
    printf("-----------------------------------------------------------------------------\n");
    for (int i = 0; i < med_count; ++i) {
        printf("%6d | %-32s | %8.2f | %5d | %4s | %7d\n",
               med_code[i], med_name[i], med_price[i], med_stock[i],
               med_is_otc[i] ? "OTC" : "RX", med_critical[i]);
    }
}

/* Mostrar medicamento por código */
void show_medicine_by_code(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int med_count
) {
    int code;
    if (!prompt_int("Ingrese código (vaciar para cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code, med_code, med_count);
    if (idx == -1) { printf("No encontrado.\n"); return; }
    printf("Código: %d\nNombre: %s\nPrecio: %.2f\nStock: %d\nTipo: %s\nStock crítico: %d\n",
           med_code[idx], med_name[idx], med_price[idx], med_stock[idx],
           med_is_otc[idx] ? "Venta libre (OTC)" : "Bajo receta (RX)", med_critical[idx]);
}

/* Editar medicamento (dueño) */
void edit_medicine(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int med_count
) {
    int code;
    if (!prompt_int("Código a editar (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code, med_code, med_count);
    if (idx == -1) { printf("Medicamento no encontrado.\n"); return; }

    char buf[MAX_INPUT];
    printf("Nombre (actual: %s) [ENTER para mantener]: ", med_name[idx]);
    read_line(buf, sizeof(buf)); trim(buf);
    if (buf[0] != '\0') { strncpy(med_name[idx], buf, MAX_NAME_LEN-1); med_name[idx][MAX_NAME_LEN-1] = '\0'; }

    double price;
    printf("Precio (actual: %.2f) [ENTER para mantener]: ", med_price[idx]);
    if (prompt_double("", &price)) med_price[idx] = price;

    int stock;
    printf("Stock (actual: %d) [ENTER para mantener]: ", med_stock[idx]);
    if (prompt_int("", &stock)) med_stock[idx] = stock;

    printf("Venta libre? (actual: %s) s/n [ENTER para mantener]: ", med_is_otc[idx] ? "s" : "n");
    read_line(buf, sizeof(buf)); trim(buf);
    if (buf[0] == 's' || buf[0] == 'S') med_is_otc[idx] = 1;
    else if (buf[0] == 'n' || buf[0] == 'N') med_is_otc[idx] = 0;

    int crit;
    printf("Stock crítico (actual: %d) [ENTER para mantener]: ", med_critical[idx]);
    if (prompt_int("", &crit)) med_critical[idx] = crit;

    printf("Medicamento actualizado.\n");
}

/* Eliminar medicamento (dueño) */
void delete_medicine(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int *med_count_ptr
) {
    int med_count = *med_count_ptr;
    int code;
    if (!prompt_int("Código a eliminar (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code, med_code, med_count);
    if (idx == -1) { printf("No encontrado.\n"); return; }

    char confirm[MAX_INPUT];
    printf("Confirma eliminación de '%s' (s/n): ", med_name[idx]);
    read_line(confirm, sizeof(confirm));
    if (tolower((unsigned char)confirm[0]) != 's') { printf("Eliminación cancelada.\n"); return; }

    for (int i = idx; i < med_count - 1; ++i) {
        med_code[i] = med_code[i+1];
        strncpy(med_name[i], med_name[i+1], MAX_NAME_LEN);
        med_price[i] = med_price[i+1];
        med_stock[i] = med_stock[i+1];
        med_is_otc[i] = med_is_otc[i+1];
        med_critical[i] = med_critical[i+1];
    }
    med_count--;
    *med_count_ptr = med_count;
    printf("Medicamento eliminado.\n");
}

/* Imprime CSV de medicamentos en pantalla (no archivo) */
void print_medicines_csv(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_critical[], int med_count
) {
    printf("Codigo,Nombre,Precio,Stock,StockCritico,VentaLibre\n");
    for (int i = 0; i < med_count; ++i) {
        char safe_name[MAX_NAME_LEN];
        strncpy(safe_name, med_name[i], MAX_NAME_LEN-1); safe_name[MAX_NAME_LEN-1] = '\0';
        for (char *p = safe_name; *p; ++p) if (*p == ',') *p = ' ';
        printf("%d,%s,%.2f,%d,%d,%d\n",
               med_code[i], safe_name, med_price[i], med_stock[i], med_critical[i], med_is_otc[i]);
    }
}

/* ----------------- Módulo Ventas / Informes ----------------- */

/* Registrar venta */
void sell_medicine(
    int med_code[], char med_name[][MAX_NAME_LEN], double med_price[],
    int med_stock[], int med_is_otc[], int med_count,
    int sale_day[], int sale_med_code[], int sale_qty[], double sale_amount[],
    char sale_dni[][32], int *sale_count_ptr
) {
    int sale_count = *sale_count_ptr;
    if (sale_count >= MAX_SALES) { printf("Límite de ventas alcanzado este mes.\n"); return; }

    int code;
    if (!prompt_int("Código de medicamento a vender (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code, med_code, med_count);
    if (idx == -1) { printf("Código no existe.\n"); return; }

    int qty;
    if (!prompt_int("Cantidad a vender: ", &qty)) return;
    if (qty <= 0) { printf("Cantidad debe ser mayor que 0.\n"); return; }
    if (qty > med_stock[idx]) { printf("Stock insuficiente.\n"); return; }

    int day;
    if (!prompt_int("Día de la venta (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Día inválido.\n"); return; }

    double total = qty * med_price[idx];

    sale_day[sale_count] = day;
    sale_med_code[sale_count] = med_code[idx];
    sale_qty[sale_count] = qty;
    sale_amount[sale_count] = total;

    if (med_is_otc[idx]) {
        strcpy(sale_dni[sale_count], "-");
    } else {
        char dnibuf[32];
        printf("Ingrese DNI del comprador (vaciar = NO registrar - advertencia legal): ");
        read_line(dnibuf, sizeof(dnibuf));
        trim(dnibuf);
        if (dnibuf[0] == '\0') {
            strcpy(sale_dni[sale_count], "-");
            printf("Advertencia: venta RX sin registro de DNI.\n");
        } else {
            strncpy(sale_dni[sale_count], dnibuf, sizeof(sale_dni[sale_count]) - 1);
            sale_dni[sale_count][sizeof(sale_dni[sale_count]) - 1] = '\0';
        }
    }

    sale_count++;
    *sale_count_ptr = sale_count;
    med_stock[idx] -= qty;

    printf("Venta registrada: $%.2f | Día %d | Quedan %d unidades en stock.\n", total, day, med_stock[idx]);
}

/* Informe mensual */
void report_monthly(int sale_day[], double sale_amount[], int sale_count) {
    if (sale_count == 0) { printf("No hay ventas registradas este mes.\n"); return; }
    double total_month = 0.0;
    int sales_per_day[DAYS_IN_MONTH + 1];
    for (int i = 0; i <= DAYS_IN_MONTH; ++i) sales_per_day[i] = 0;
    for (int i = 0; i < sale_count; ++i) {
        int d = sale_day[i];
        if (d >= 1 && d <= DAYS_IN_MONTH) sales_per_day[d] += 1;
        total_month += sale_amount[i];
    }
    printf("===== Informe mensual =====\n");
    printf("Total ventas (importe): $%.2f\n", total_month);
    printf("Ventas por día (día: cantidad de ventas):\n");
    for (int d = 1; d <= DAYS_IN_MONTH; ++d) if (sales_per_day[d] > 0) printf("Día %2d: %d\n", d, sales_per_day[d]);
    printf("Total operaciones registradas en el mes: %d\n", sale_count);
}

/* Informe de un día específico */
void report_day(int sale_day[], double sale_amount[], int sale_count) {
    int day;
    if (!prompt_int("Ingrese día a consultar (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Día inválido.\n"); return; }
    double total_day = 0.0;
    int count_day = 0;
    for (int i = 0; i < sale_count; ++i) if (sale_day[i] == day) { total_day += sale_amount[i]; count_day++; }
    printf("Informe día %d: %d operaciones | Total importe: $%.2f\n", day, count_day, total_day);
}

/* Imprime CSV de ventas en pantalla */
void print_sales_csv(int sale_day[], int sale_med_code[], int sale_qty[], double sale_amount[], char sale_dni[][32], int sale_count) {
    printf("Dia,CodigoMedicamento,Cantidad,Importe,DNI\n");
    for (int i = 0; i < sale_count; ++i) {
        printf("%d,%d,%d,%.2f,%s\n", sale_day[i], sale_med_code[i], sale_qty[i], sale_amount[i], sale_dni[i]);
    }
}

/* Mostrar registros RX (ventas con receta) */
void show_rx_records(int sale_day[], int sale_med_code[], int sale_qty[], char sale_dni[][32], int sale_count,
                     int med_code[], int med_count, int med_is_otc[]) {
    int found = 0;
    printf("DIA | Codigo | Cant | DNI\n");
    printf("-------------------------\n");
    for (int i = 0; i < sale_count; ++i) {
        int medidx = find_med_index_by_code(sale_med_code[i], med_code, med_count);
        int is_rx = 1;
        if (medidx != -1) is_rx = !med_is_otc[medidx];
        if (is_rx) {
            found = 1;
            printf("%3d | %6d | %4d | %s\n", sale_day[i], sale_med_code[i], sale_qty[i], sale_dni[i]);
        }
    }
    if (!found) printf("No hay registros RX.\n");
}

/* Reporte stock crítico */
void report_stock_critical(int med_code[], char med_name[][MAX_NAME_LEN], int med_stock[], int med_critical[], int med_count) {
    int found = 0;
    printf("Medicamentos en o por debajo del stock crítico:\n");
    for (int i = 0; i < med_count; ++i) {
        if (med_stock[i] <= med_critical[i]) {
            found = 1;
            printf("Código %d | %s | Stock: %d | Crítico: %d\n",
                   med_code[i], med_name[i], med_stock[i], med_critical[i]);
        }
    }
    if (!found) printf("Ningún medicamento está por debajo del stock crítico.\n");
}

/* Reset mensual */
void reset_month(int *sale_count_ptr) {
    *sale_count_ptr = 0;
    printf("Datos de ventas del mes borrados.\n");
}

/* Cambiar contraseña del dueño (recibe owner_password desde main) */
void change_owner_password(char owner_password[], size_t pass_len) {
    char current[64], newp[64], conf[64];
    printf("Ingrese contraseña actual (vaciar cancelar): ");
    read_password(current, sizeof(current));
    if (current[0] == '\0') { printf("Cancelado.\n"); return; }
    if (strcmp(current, owner_password) != 0) { printf("Contraseña incorrecta.\n"); return; }
    printf("Nueva contraseña: ");
    read_password(newp, sizeof(newp));
    if (newp[0] == '\0') { printf("Cancelado.\n"); return; }
    printf("Confirme nueva contraseña: ");
    read_password(conf, sizeof(conf));
    if (strcmp(newp, conf) != 0) { printf("No coinciden. Cancelado.\n"); return; }
    strncpy(owner_password, newp, pass_len - 1);
    owner_password[pass_len - 1] = '\0';
    printf("Contraseña actualizada.\n");
}

/* Autenticación del dueño (recibe owner_password desde main) */
int authenticate_owner(const char owner_password[]) {
    char buf[64];
    printf("Ingrese contraseña del dueño (vaciar cancelar): ");
    read_password(buf, sizeof(buf));
    if (buf[0] == '\0') return 0;
    if (strcmp(buf, owner_password) == 0) return 1;
    printf("Contraseña incorrecta.\n");
    return 0;
}

/* ----------------- Menú / main ----------------- */

void show_header(void) {
    printf("=========================================\n");
    printf("Sistema de Farmacia (memoria volátil)\n");
    printf("ATENCIÓN: NO usa archivos. Todo se pierde si se corta la luz.\n");
    printf("=========================================\n");
}

int main(void) {
    setbuf(stdout, NULL);

    /* Todas las variables declaradas aquí (NINGUNA global) */
    int med_count = 0;
    int med_code[MAX_MEDICINES];
    char med_name[MAX_MEDICINES][MAX_NAME_LEN];
    double med_price[MAX_MEDICINES];
    int med_stock[MAX_MEDICINES];
    int med_is_otc[MAX_MEDICINES];
    int med_critical[MAX_MEDICINES];

    int sale_count = 0;
    int sale_day[MAX_SALES];
    int sale_med_code[MAX_SALES];
    int sale_qty[MAX_SALES];
    double sale_amount[MAX_SALES];
    char sale_dni[MAX_SALES][32];

    char owner_password[64] = "admin123"; /* contraseña inicial (en memoria) */

    int running = 1;
    while (running) {
        show_header();
        printf("Opciones:\n");
        printf(" 1) Agregar medicamento\n");
        printf(" 2) Listar medicamentos\n");
        printf(" 3) Mostrar medicamento por código\n");
        printf(" 4) Editar medicamento (dueño)\n");
        printf(" 5) Eliminar medicamento (dueño)\n");
        printf(" 6) Vender medicamento\n");
        printf(" 7) Informe mensual (pantalla)\n");
        printf(" 8) Informe día (dueño)\n");
        printf(" 9) Mostrar registros RX (dueño)\n");
        printf("10) Reporte stock crítico (dueño)\n");
        printf("11) Reiniciar mes (dueño)\n");
        printf("12) Cambiar contraseña (dueño)\n");
        printf("13) Mostrar CSV medicamentos (pantalla)\n");
        printf("14) Mostrar CSV ventas (pantalla)\n");
        printf(" 0) Salir\n");
        printf("---------------------------------\n");

        int option;
        if (!prompt_int("Seleccione opción: ", &option)) { printf("Entrada vacía. Volviendo al menú.\n"); continue; }

        switch (option) {
            case 1:
                add_medicine(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, &med_count);
                break;
            case 2:
                list_medicines(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, med_count);
                break;
            case 3:
                show_medicine_by_code(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, med_count);
                break;
            case 4:
                if (authenticate_owner(owner_password)) edit_medicine(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, med_count);
                else printf("No autorizado.\n");
                break;
            case 5:
                if (authenticate_owner(owner_password)) delete_medicine(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, &med_count);
                else printf("No autorizado.\n");
                break;
            case 6:
                sell_medicine(med_code, med_name, med_price, med_stock, med_is_otc, med_count,
                              sale_day, sale_med_code, sale_qty, sale_amount, sale_dni, &sale_count);
                break;
            case 7:
                report_monthly(sale_day, sale_amount, sale_count);
                break;
            case 8:
                if (authenticate_owner(owner_password)) report_day(sale_day, sale_amount, sale_count);
                else printf("No autorizado.\n");
                break;
            case 9:
                if (authenticate_owner(owner_password)) show_rx_records(sale_day, sale_med_code, sale_qty, sale_dni, sale_count, med_code, med_count, med_is_otc);
                else printf("No autorizado.\n");
                break;
            case 10:
                if (authenticate_owner(owner_password)) report_stock_critical(med_code, med_name, med_stock, med_critical, med_count);
                else printf("No autorizado.\n");
                break;
            case 11:
                if (authenticate_owner(owner_password)) {
                    char c[MAX_INPUT];
                    printf("CONFIRME: desea borrar TODOS los datos de ventas del mes actual? (s/n): ");
                    read_line(c, sizeof(c));
                    if (tolower((unsigned char)c[0]) == 's') reset_month(&sale_count);
                    else printf("Operación cancelada.\n");
                } else printf("No autorizado.\n");
                break;
            case 12:
                if (authenticate_owner(owner_password)) change_owner_password(owner_password, sizeof(owner_password));
                else printf("No autorizado.\n");
                break;
            case 13:
                print_medicines_csv(med_code, med_name, med_price, med_stock, med_is_otc, med_critical, med_count);
                break;
            case 14:
                print_sales_csv(sale_day, sale_med_code, sale_qty, sale_amount, sale_dni, sale_count);
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("Opción inválida.\n");
                break;
        }

        printf("\nPresione ENTER para continuar...");
        char tmp[MAX_INPUT];
        read_line(tmp, sizeof(tmp));
    }

    printf("Saliendo. Todos los datos en memoria serán perdidos.\n");
    return 0;
}
/* farmacia_no_files_no_structs.c
   Sistema de Farmacia sin archivos y sin structs/typedef.
   - Todos los datos en memoria usando arrays paralelos.
   - "Exportar CSV" imprime CSV por pantalla (no escribe archivos).
   - Compilar: gcc -std=c11 -O2 -Wall farmacia_no_files_no_structs.c -o farmacia
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

/* ------------- CONFIG ------------- */
#define MAX_MEDICINES 200
#define MAX_NAME_LEN 64
#define MAX_SALES 5000
#define MAX_INPUT 128
#define DAYS_IN_MONTH 31

/* Contraseña inicial del dueño (en memoria). Se muestra en comentario. */
/* contrasena inicial: admin123 */
static char owner_password[64] = "admin123";

/* ------------- DATOS: arrays paralelos para medicamentos ------------- */
static int med_count = 0;
static int med_code[MAX_MEDICINES];
static char med_name[MAX_MEDICINES][MAX_NAME_LEN];
static double med_price[MAX_MEDICINES];
static int med_stock[MAX_MEDICINES];
static int med_is_otc[MAX_MEDICINES];    /* 1 = venta libre, 0 = bajo receta */
static int med_critical[MAX_MEDICINES];

/* ------------- DATOS: arrays paralelos para ventas ------------- */
static int sale_count = 0;
static int sale_day[MAX_SALES];        /* dia 1..31 */
static int sale_med_code[MAX_SALES];   /* codigo del medicamento */
static int sale_qty[MAX_SALES];        /* cantidad vendida en la operacion */
static double sale_amount[MAX_SALES];  /* importe total de la operacion */
static char sale_dni[MAX_SALES][32];   /* dni del comprador si RX, '-' si OTC */

/* ------------- UTILIDADES DE ENTRADA ------------- */
static void read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len && buf[len-1] == '\n') buf[len-1] = '\0';
}

static void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) { s[len-1] = '\0'; len--; }
}

/* Leer contraseña oculta (Linux/macOS/Windows compatibilidad mínima).
   En Windows con MinGW la funcion getchar() seguirá mostrando, pero dejamos esta version portable.
*/
static void read_password(char *out, size_t maxlen) {
    size_t i = 0;
#ifdef _WIN32
    /* Windows simple: no ocultamos (puede mejorarse con conio.h en Windows) */
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
    return;
#endif
}

/* Leer entero con prompt, retorna 1 si ok, 0 si input vacio */
static int prompt_int(const char *prompt, int *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        trim(buf);
        if (buf[0] == '\0') return 0;
        char *end;
        long v = strtol(buf, &end, 10);
        if (end == buf || *end != '\0') {
            printf("Entrada invalida. Ingrese un numero entero.\n");
            continue;
        }
        *out_val = (int)v;
        return 1;
    }
}

/* Leer double con prompt */
static int prompt_double(const char *prompt, double *out_val) {
    char buf[MAX_INPUT];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        trim(buf);
        if (buf[0] == '\0') return 0;
        char *end;
        double v = strtod(buf, &end);
        if (end == buf || *end != '\0') {
            printf("Entrada invalida. Ingrese un numero (puede tener decimales).\n");
            continue;
        }
        *out_val = v;
        return 1;
    }
}

/* ------------- BÚSQUEDAS ------------- */
static int find_med_index_by_code(int code) {
    for (int i = 0; i < med_count; ++i) if (med_code[i] == code) return i;
    return -1;
}

/* ------------- FUNCIONES DE MEDICAMENTOS ------------- */
static void add_medicine(void) {
    if (med_count >= MAX_MEDICINES) { printf("Limite de catalogo alcanzado.\n"); return; }

    int code;
    if (!prompt_int("Ingrese codigo (vaciar para cancelar): ", &code)) return;
    if (find_med_index_by_code(code) != -1) { printf("Codigo ya existente.\n"); return; }

    char name[MAX_NAME_LEN];
    printf("Nombre: ");
    read_line(name, sizeof(name));
    trim(name);
    if (name[0] == '\0') { printf("Nombre vacio. Cancelado.\n"); return; }

    double price;
    if (!prompt_double("Precio (ej: 123.45): ", &price)) return;
    if (price < 0) { printf("Precio invalido.\n"); return; }

    int stock;
    if (!prompt_int("Stock inicial: ", &stock)) return;
    if (stock < 0) { printf("Stock invalido.\n"); return; }

    char otcbuf[MAX_INPUT];
    int is_otc = -1;
    while (is_otc == -1) {
        printf("Venta libre? (s/n): ");
        read_line(otcbuf, sizeof(otcbuf));
        trim(otcbuf);
        if (otcbuf[0] == '\0') { printf("Cancelado.\n"); return; }
        if (tolower((unsigned char)otcbuf[0]) == 's') is_otc = 1;
        else if (tolower((unsigned char)otcbuf[0]) == 'n') is_otc = 0;
        else printf("Respuesta invalida, ingrese s o n.\n");
    }

    int crit;
    if (!prompt_int("Stock critico: ", &crit)) return;
    if (crit < 0) { printf("Stock critico invalido.\n"); return; }

    int idx = med_count++;
    med_code[idx] = code;
    strncpy(med_name[idx], name, MAX_NAME_LEN-1); med_name[idx][MAX_NAME_LEN-1] = '\0';
    med_price[idx] = price;
    med_stock[idx] = stock;
    med_is_otc[idx] = is_otc;
    med_critical[idx] = crit;

    printf("Medicamento agregado (indice %d).\n", idx);
}

static void list_medicines(void) {
    if (med_count == 0) { printf("No hay medicamentos registrados.\n"); return; }
    printf("Codigo | Nombre                           | Precio   | Stock | Tipo | Critico\n");
    printf("-----------------------------------------------------------------------------\n");
    for (int i = 0; i < med_count; ++i) {
        printf("%6d | %-32s | %8.2f | %5d | %4s | %7d\n",
               med_code[i],
               med_name[i],
               med_price[i],
               med_stock[i],
               med_is_otc[i] ? "OTC" : "RX",
               med_critical[i]);
    }
}

static void show_medicine_by_code(void) {
    int code;
    if (!prompt_int("Ingrese codigo (vaciar para cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
    if (idx == -1) { printf("No encontrado.\n"); return; }
    printf("Codigo: %d\nNombre: %s\nPrecio: %.2f\nStock: %d\nTipo: %s\nCritico: %d\n",
           med_code[idx], med_name[idx], med_price[idx], med_stock[idx],
           med_is_otc[idx] ? "Venta libre (OTC)" : "Bajo receta (RX)", med_critical[idx]);
}

static void edit_medicine(void) {
    int code;
    if (!prompt_int("Codigo a editar (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
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
    printf("Stock critico (actual: %d) [ENTER para mantener]: ", med_critical[idx]);
    if (prompt_int("", &crit)) med_critical[idx] = crit;

    printf("Medicamento actualizado.\n");
}

static void delete_medicine(void) {
    int code;
    if (!prompt_int("Codigo a eliminar (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
    if (idx == -1) { printf("No encontrado.\n"); return; }

    char confirm[MAX_INPUT];
    printf("Confirma eliminacion de '%s' (s/n): ", med_name[idx]);
    read_line(confirm, sizeof(confirm));
    if (tolower((unsigned char)confirm[0]) != 's') { printf("Eliminacion cancelada.\n"); return; }

    for (int i = idx; i < med_count - 1; ++i) {
        med_code[i] = med_code[i+1];
        strncpy(med_name[i], med_name[i+1], MAX_NAME_LEN);
        med_price[i] = med_price[i+1];
        med_stock[i] = med_stock[i+1];
        med_is_otc[i] = med_is_otc[i+1];
        med_critical[i] = med_critical[i+1];
    }
    med_count--;
    printf("Medicamento eliminado.\n");
}

/* Imprime CSV de medicamentos por pantalla (no escribe archivo) */
static void print_medicines_csv(void) {
    printf("Codigo,Nombre,Precio,Stock,StockCritico,VentaLibre\n");
    for (int i = 0; i < med_count; ++i) {
        /* Reemplazar comas en los nombres para no romper CSV básico */
        char safe_name[MAX_NAME_LEN];
        strncpy(safe_name, med_name[i], MAX_NAME_LEN-1); safe_name[MAX_NAME_LEN-1] = '\0';
        for (char *p = safe_name; *p; ++p) if (*p == ',') *p = ' ';
        printf("%d,%s,%.2f,%d,%d,%d\n",
               med_code[i], safe_name, med_price[i], med_stock[i], med_critical[i], med_is_otc[i]);
    }
}

/* ------------- FUNCIONES DE VENTAS E INFORMES ------------- */
static void sell_medicine(void) {
    if (sale_count >= MAX_SALES) { printf("Limite de ventas alcanzado este mes.\n"); return; }

    int code;
    if (!prompt_int("Codigo a vender (vaciar cancelar): ", &code)) return;
    int idx = find_med_index_by_code(code);
    if (idx == -1) { printf("Codigo no existe.\n"); return; }

    int qty;
    if (!prompt_int("Cantidad a vender: ", &qty)) return;
    if (qty <= 0) { printf("Cantidad invalida.\n"); return; }
    if (qty > med_stock[idx]) { printf("Stock insuficiente.\n"); return; }

    int day;
    if (!prompt_int("Dia de la venta (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Dia invalido.\n"); return; }

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
            /* registramos '-' pero avisamos */
            strcpy(sale_dni[sale_count], "-");
            printf("Advertencia: venta RX sin registro de DNI.\n");
        } else {
            strncpy(sale_dni[sale_count], dnibuf, sizeof(sale_dni[sale_count]) - 1);
            sale_dni[sale_count][sizeof(sale_dni[sale_count]) - 1] = '\0';
        }
    }

    sale_count++;
    med_stock[idx] -= qty;

    printf("Venta registrada: $%.2f | Dia %d | Quedan %d unidades.\n", total, day, med_stock[idx]);
}

/* Informe mensual: total en pesos y ventas por dia */
static void report_monthly(void) {
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
    printf("Total importe: $%.2f\n", total_month);
    printf("Ventas por dia (dia: cantidad):\n");
    for (int d = 1; d <= DAYS_IN_MONTH; ++d) if (sales_per_day[d] > 0) printf("Dia %2d: %d\n", d, sales_per_day[d]);
    printf("Operaciones totales en el mes: %d\n", sale_count);
}

/* Informe de un dia especifico */
static void report_day(void) {
    int day;
    if (!prompt_int("Ingrese dia a consultar (1-31): ", &day)) return;
    if (day < 1 || day > DAYS_IN_MONTH) { printf("Dia invalido.\n"); return; }
    double total_day = 0.0;
    int count_day = 0;
    for (int i = 0; i < sale_count; ++i) if (sale_day[i] == day) { total_day += sale_amount[i]; count_day++; }
    printf("Informe dia %d: %d operaciones | Total importe: $%.2f\n", day, count_day, total_day);
}

/* Mostrar ventas (CSV) por pantalla (no archivo) */
static void print_sales_csv(void) {
    printf("Dia,CodigoMedicamento,Cantidad,Importe,DNI\n");
    for (int i = 0; i < sale_count; ++i) {
        printf("%d,%d,%d,%.2f,%s\n", sale_day[i], sale_med_code[i], sale_qty[i], sale_amount[i], sale_dni[i]);
    }
}

/* Mostrar registros RX (ventas con receta) */
static void show_rx_records(void) {
    int found = 0;
    printf("DIA | Codigo | Cant | DNI\n");
    printf("-------------------------\n");
    for (int i = 0; i < sale_count; ++i) {
        /* identificar si el medicamento era RX: buscar med index por codigo */
        int medidx = find_med_index_by_code(sale_med_code[i]);
        int is_rx = 1;
        if (medidx != -1) is_rx = !med_is_otc[medidx];
        if (is_rx) {
            found = 1;
            printf("%3d | %6d | %4d | %s\n", sale_day[i], sale_med_code[i], sale_qty[i], sale_dni[i]);
        }
    }
    if (!found) printf("No hay registros RX.\n");
}

/* Reporte de stock critico (solo dueno) */
static void report_stock_critical(void) {
    int found = 0;
    printf("Medicamentos en o por debajo del stock critico:\n");
    for (int i = 0; i < med_count; ++i) {
        if (med_stock[i] <= med_critical[i]) {
            found = 1;
            printf("Codigo %d | %s | Stock: %d | Critico: %d\n",
                   med_code[i], med_name[i], med_stock[i], med_critical[i]);
        }
    }
    if (!found) printf("Ningun medicamento esta por debajo del stock critico.\n");
}

/* Reset mensual: borrar ventas */
static void reset_month(void) {
    sale_count = 0;
    printf("Datos de ventas del mes borrados.\n");
}

/* Cambiar contrasena del dueño */
static void change_owner_password(void) {
    char current[64], newp[64], conf[64];
    printf("Ingrese contrasena actual (vaciar cancelar): ");
    read_password(current, sizeof(current));
    if (current[0] == '\0') { printf("Cancelado.\n"); return; }
    if (strcmp(current, owner_password) != 0) { printf("Contrasena incorrecta.\n"); return; }
    printf("Nueva contrasena: ");
    read_password(newp, sizeof(newp));
    if (newp[0] == '\0') { printf("Cancelado.\n"); return; }
    printf("Confirme nueva contrasena: ");
    read_password(conf, sizeof(conf));
    if (strcmp(newp, conf) != 0) { printf("No coinciden. Cancelado.\n"); return; }
    strncpy(owner_password, newp, sizeof(owner_password)-1); owner_password[sizeof(owner_password)-1] = '\0';
    printf("Contrasena actualizada.\n");
}

/* Autenticacion del dueño */
static int authenticate_owner(void) {
    char buf[64];
    printf("Ingrese contrasena del dueno (vaciar cancelar): ");
    read_password(buf, sizeof(buf));
    if (buf[0] == '\0') return 0;
    if (strcmp(buf, owner_password) == 0) return 1;
    printf("Contrasena incorrecta.\n");
    return 0;
}

/* ------------- MENU PRINCIPAL ------------- */
static void show_header(void) {
    printf("=========================================\n");
    printf("Sistema de Farmacia (memoria volatile)\n");
    printf("ATENCION: NO usa archivos. Todo se pierde si se corta la luz.\n");
    printf("=========================================\n");
}

int main(void) {
    setbuf(stdout, NULL);
    int running = 1;
    while (running) {
        show_header();
        printf("Opciones:\n");
        printf(" 1) Agregar medicamento\n");
        printf(" 2) Listar medicamentos\n");
        printf(" 3) Mostrar medicamento por codigo\n");
        printf(" 4) Editar medicamento (dueno)\n");
        printf(" 5) Eliminar medicamento (dueno)\n");
        printf(" 6) Vender medicamento\n");
        printf(" 7) Informe mensual (pantalla)\n");
        printf(" 8) Informe dia (dueno)\n");
        printf(" 9) Mostrar registros RX (dueno)\n");
        printf("10) Reporte stock critico (dueno)\n");
        printf("11) Reiniciar mes (dueno)\n");
        printf("12) Cambiar contrasena (dueno)\n");
        printf("13) Mostrar CSV medicamentos (pantalla)\n");
        printf("14) Mostrar CSV ventas (pantalla)\n");
        printf(" 0) Salir\n");
        printf("---------------------------------\n");

        int option;
        if (!prompt_int("Seleccione opcion: ", &option)) { printf("Entrada vacia. Volviendo al menu.\n"); continue; }

        switch (option) {
            case 1: add_medicine(); break;
            case 2: list_medicines(); break;
            case 3: show_medicine_by_code(); break;
            case 4:
                if (authenticate_owner()) edit_medicine();
                else printf("No autorizado.\n");
                break;
            case 5:
                if (authenticate_owner()) delete_medicine();
                else printf("No autorizado.\n");
                break;
            case 6: sell_medicine(); break;
            case 7: report_monthly(); break;
            case 8:
                if (authenticate_owner()) report_day();
                else printf("No autorizado.\n");
                break;
            case 9:
                if (authenticate_owner()) show_rx_records();
                else printf("No autorizado.\n");
                break;
            case 10:
                if (authenticate_owner()) report_stock_critical();
                else printf("No autorizado.\n");
                break;
            case 11:
                if (authenticate_owner()) {
                    char c[MAX_INPUT];
                    printf("CONFIRME: desea borrar TODOS los datos de ventas del mes actual? (s/n): ");
                    read_line(c, sizeof(c));
                    if (tolower((unsigned char)c[0]) == 's') reset_month();
                    else printf("Operacion cancelada.\n");
                } else printf("No autorizado.\n");
                break;
            case 12:
                if (authenticate_owner()) change_owner_password();
                else printf("No autorizado.\n");
                break;
            case 13:
                /* Imprime CSV de medicamentos en pantalla (sin usar archivos) */
                print_medicines_csv();
                break;
            case 14:
                /* Imprime CSV de ventas en pantalla (sin usar archivos) */
                print_sales_csv();
                break;
            case 0: running = 0; break;
            default: printf("Opcion invalida.\n"); break;
        }

        printf("\nPresione ENTER para continuar...");
        char tmp[MAX_INPUT];
        read_line(tmp, sizeof(tmp));
    }

    printf("Saliendo. Todos los datos en memoria seran perdidos.\n");
    return 0;
}

#include <stdio.h>

#define MAX_FILAS   10
#define MAX_COLS    10
#define TOTAL_OBJ   2
#define LIM_TURNOS  300
#define MAX_BLOQ    3

#define CHAR_VACIO  ' '
#define CHAR_BASE   'B'
#define CHAR_OBJ    'O'
#define CHAR_OBS    'X'
#define CHAR_RIESG  '!'

#define ST_NORMAL   0
#define ST_PRECAVE  1
#define ST_PELIGRO  2

#define EST_INICIO  0
#define EST_CURSO   1
#define EST_RETORNO 2
#define EST_FALLO   3

#define MOT_NADA    0
#define MOT_EXITO   1
#define MOT_CLIMAB  2
#define MOT_CLIMAV  3
#define MOT_SISMOC  4
#define MOT_SISMOT  5
#define MOT_BATBAJ1 6
#define MOT_BATBAJ2 7
#define MOT_TRABADO 8

typedef struct {
    int   pos_f;
    int   pos_c;
    char  etiqueta[32];
    int   nivel_p;
    int   listo;
    int   colapso;
} InfoObj;

typedef struct {
    int   pos_f;
    int   pos_c;
    float nivel_bat;
    float peso_kg;
    int   modo;
    int   idx_actual;
    int   intentos;
} InfoDron;

typedef struct {
    float aire;
    float agua;
    int   aviso;
} InfoClima;

char       tablero[MAX_FILAS][MAX_COLS];
InfoDron   mi_dron;
InfoClima  mi_clima;
InfoObj    vector_obj[TOTAL_OBJ];

int base_f = 2;
int base_c = 2;
int cont_t = 0;
int metas_ok = 0;
int seed_gen = 1337;

int calcular_aleatorio(int limite) {
    seed_gen = seed_gen * 214013 + 2531011;
    if (seed_gen < 0) seed_gen = -seed_gen;
    return seed_gen % limite;
}

int mi_abs(int val) {
    if (val < 0) return -val;
    return val;
}

int calcular_distancia(int x1, int y1, int x2, int y2) {
    return mi_abs(x1 - x2) + mi_abs(y1 - y2);
}

int buscar_siguiente(void) {
    int k, seleccionado = -1;
    for (k = 0; k < TOTAL_OBJ; k++) {
        if (!vector_obj[k].listo) {
            if (seleccionado == -1 || vector_obj[k].nivel_p < vector_obj[seleccionado].nivel_p)
                seleccionado = k;
        }
    }
    return seleccionado;
}

int verificar_todos(void) {
    int k;
    for (k = 0; k < TOTAL_OBJ; k++)
        if (!vector_obj[k].listo) return 0;
    return 1;
}

void copiar_texto(char *destino, char *origen) {
    int idx = 0;
    while (origen[idx] != '\0') {
        destino[idx] = origen[idx];
        idx++;
    }
    destino[idx] = '\0';
}

void mostrar_aviso(void) {
    if      (mi_clima.aviso == ST_PELIGRO)  printf("PELIGRO");
    else if (mi_clima.aviso == ST_PRECAVE) printf("PRECAUCION");
    else                                   printf("NORMAL");
}

void mostrar_modo_dron(void) {
    if      (mi_dron.modo == EST_CURSO)   printf("Volando");
    else if (mi_dron.modo == EST_RETORNO) printf("Regresando");
    else if (mi_dron.modo == EST_FALLO)   printf("Emergencia");
    else                                  printf("En base");
}

void mostrar_motivo(int motivo) {
    if      (motivo == MOT_EXITO)   printf("Mision completada con exito");
    else if (motivo == MOT_CLIMAB)  printf("Despegue denegado por clima");
    else if (motivo == MOT_CLIMAV)  printf("Clima peligroso en vuelo");
    else if (motivo == MOT_SISMOC)  printf("Replica en zona de colapso");
    else if (motivo == MOT_SISMOT)  printf("Replica sismica en trayecto");
    else if (motivo == MOT_BATBAJ1) printf("Bateria critica (<10%%)");
    else if (motivo == MOT_BATBAJ2) printf("Bateria insuficiente para volver");
    else if (motivo == MOT_TRABADO) printf("Bloqueo total de ruta");
    else                            printf("Desconocida");
}

void mostrar_tablero(void) {
    int r, c;
    printf("    ");
    for (c = 0; c < MAX_COLS; c++) printf(" %d ", c);
    printf("\n");

    for (r = 0; r < MAX_FILAS; r++) {
        printf(" %d  ", r);
        for (c = 0; c < MAX_COLS; c++) {
            if (r == mi_dron.pos_f && c == mi_dron.pos_c &&
                mi_dron.modo != EST_INICIO &&
                mi_dron.modo != EST_FALLO) {
                printf("[D]");
            } else {
                char letra = tablero[r][c];
                if (letra == CHAR_VACIO) printf("[ ]");
                else                     printf("[%c]", letra);
            }
        }
        if (r == 5) printf("  <- Hospital");
        if (r == 7) printf("  <- Colapso");
        printf("\n");
    }
    printf("\n");
}

void mostrar_estado(void) {
    printf("[T:%d] P:(%d,%d) Bat:%.0f%% | V:%.0f L:%.0f | ",
           cont_t, mi_dron.pos_f, mi_dron.pos_c, mi_dron.nivel_bat,
           mi_clima.aire, mi_clima.agua);
    mostrar_aviso();
    printf(" | ");
    mostrar_modo_dron();
    printf("\n");
}

void configurar_tablero(void) {
    int r, c;
    for (r = 0; r < MAX_FILAS; r++)
        for (c = 0; c < MAX_COLS; c++)
            tablero[r][c] = CHAR_VACIO;

    tablero[2][2] = CHAR_BASE;
    tablero[4][5] = CHAR_OBS;
    tablero[4][6] = CHAR_OBS;
    tablero[5][7] = CHAR_OBJ;
    tablero[6][4] = CHAR_OBS;
    tablero[7][3] = CHAR_OBJ;
    tablero[8][6] = CHAR_RIESG;
}

void configurar_objetivos(void) {
    char str0[] = "Hospital La Guaira";
    char str1[] = "Edificio Colapsado";

    vector_obj[0].pos_f = 5; vector_obj[0].pos_c = 7; vector_obj[0].nivel_p = 1; vector_obj[0].listo = 0; vector_obj[0].colapso = 0;
    copiar_texto(vector_obj[0].etiqueta, str0);

    vector_obj[1].pos_f = 7; vector_obj[1].pos_c = 3; vector_obj[1].nivel_p = 2; vector_obj[1].listo = 0; vector_obj[1].colapso = 1;
    copiar_texto(vector_obj[1].etiqueta, str1);
}

void configurar_dron(void) {
    mi_dron.pos_f = base_f; mi_dron.pos_c = base_c; mi_dron.nivel_bat = 100.0;
    mi_dron.peso_kg = 3.0; mi_dron.modo = EST_INICIO; mi_dron.idx_actual = -1; mi_dron.intentos = 0;
}

void configurar_clima(void) {
    mi_clima.aire = 15.0; mi_clima.agua = 5.0; mi_clima.aviso = ST_NORMAL;
}

void revisar_alerta(void) {
    if (mi_clima.aire > 60.0 || mi_clima.agua > 50.0)
        mi_clima.aviso = ST_PELIGRO;
    else if (mi_clima.aire >= 30.0 || mi_clima.agua >= 20.0)
        mi_clima.aviso = ST_PRECAVE;
    else
        mi_clima.aviso = ST_NORMAL;
}

void actualizar_clima(void) {
    mi_clima.aire += (float)(calcular_aleatorio(15) - 7);
    mi_clima.agua += (float)(calcular_aleatorio(9) - 4);
    if (mi_clima.aire < 0.0)  mi_clima.aire = 0.0;
    if (mi_clima.agua < 0.0)  mi_clima.agua = 0.0;
    if (mi_clima.aire > 85.0) mi_clima.aire = 85.0;
    if (mi_clima.agua > 65.0) mi_clima.agua = 65.0;
    revisar_alerta();
}

float calcular_gasto(void) {
    float gasto = 2.0 + (mi_dron.peso_kg * 0.4) + (mi_clima.aire * 0.03) + (mi_clima.agua * 0.05);
    if (mi_clima.aviso == ST_PRECAVE) gasto *= 1.3;
    return gasto;
}

float bat_para_regresar(void) {
    return (float)calcular_distancia(mi_dron.pos_f, mi_dron.pos_c, base_f, base_c) * calcular_gasto() * 1.25;
}

int mover_hacia_destino(int destino_f, int destino_c) {
    int dif_f = 0, dif_c = 0, nueva_f, nueva_c, i;
    if      (destino_f < mi_dron.pos_f) dif_f = -1;
    else if (destino_f > mi_dron.pos_f) dif_f =  1;
    if (dif_f == 0) {
        if      (destino_c < mi_dron.pos_c) dif_c = -1;
        else if (destino_c > mi_dron.pos_c) dif_c =  1;
    }

    nueva_f = mi_dron.pos_f + dif_f; nueva_c = mi_dron.pos_c + dif_c;
    if (nueva_f >= 0 && nueva_f < MAX_FILAS && nueva_c >= 0 && nueva_c < MAX_COLS && tablero[nueva_f][nueva_c] != CHAR_OBS) {
        mi_dron.pos_f = nueva_f; mi_dron.pos_c = nueva_c; mi_dron.intentos = 0;
        return 1;
    }

    printf("[T:%d] Obstaculo detectado. Buscando ruta alterna...\n", cont_t);
    {
        int mat_dir[4][2], cant_dir = 0;
        if (dif_f != 0) {
            mat_dir[cant_dir][0] = dif_f; mat_dir[cant_dir][1] = 0; cant_dir++;
            mat_dir[cant_dir][0] = 0;     mat_dir[cant_dir][1] = 1; cant_dir++;
            mat_dir[cant_dir][0] = 0;     mat_dir[cant_dir][1] = -1;cant_dir++;
            mat_dir[cant_dir][0] = -dif_f;mat_dir[cant_dir][1] = 0; cant_dir++;
        } else {
            mat_dir[cant_dir][0] = 0;     mat_dir[cant_dir][1] = dif_c; cant_dir++;
            mat_dir[cant_dir][0] = -1;    mat_dir[cant_dir][1] = 0;     cant_dir++;
            mat_dir[cant_dir][0] = 1;     mat_dir[cant_dir][1] = 0;     cant_dir++;
            mat_dir[cant_dir][0] = 0;     mat_dir[cant_dir][1] = -dif_c;cant_dir++;
        }

        for (i = 1; i < cant_dir; i++) {
            nueva_f = mi_dron.pos_f + mat_dir[i][0];
            nueva_c = mi_dron.pos_c + mat_dir[i][1];
            if (nueva_f >= 0 && nueva_f < MAX_FILAS && nueva_c >= 0 && nueva_c < MAX_COLS && tablero[nueva_f][nueva_c] != CHAR_OBS) {
                mi_dron.pos_f = nueva_f; mi_dron.pos_c = nueva_c; mi_dron.intentos = 0;
                return 1;
            }
        }
    }

    mi_dron.intentos++;
    printf("[T:%d] Bloqueo detectado (%d/%d)\n", cont_t, mi_dron.intentos, MAX_BLOQ);
    return 0;
}

int evaluar_sismo(void) {
    int probabilidad = (tablero[mi_dron.pos_f][mi_dron.pos_c] == CHAR_RIESG) ? 40 : 8;
    return calcular_aleatorio(100) < probabilidad;
}

void ejecutar_simulacion(void) {
    int flag_activo = 1, paquete_entregado = 0, motivo_final = MOT_NADA;

    printf("\n");
    printf("     SIMULADOR DE DRON - RESPUESTA SISMICA        \n");
    printf("\n");
    printf("Mision: Entregar %.0f kg | Tablero inicial:\n\n", mi_dron.peso_kg);
    mostrar_tablero();

    if (mi_clima.aire > 50.0 || mi_clima.agua > 40.0) {
        printf("Despegue denegado por clima adverso.\n");
        motivo_final = MOT_CLIMAB;
        flag_activo = 0;
    } else {
        printf("Iniciando mision...\n\n");
        mi_dron.modo = EST_CURSO;
        mi_dron.idx_actual = buscar_siguiente();
    }

    while (flag_activo && cont_t < LIM_TURNOS) {
        cont_t++;
        actualizar_clima();

        mi_dron.nivel_bat -= (mi_dron.modo == EST_RETORNO) ? (2.0 + mi_clima.aire * 0.02) : calcular_gasto();
        if (mi_dron.nivel_bat < 0.0) mi_dron.nivel_bat = 0.0;

        if (mi_dron.nivel_bat < 10.0) {
            printf("[T:%d] BATERIA CRITICA. Aterrizaje forzoso.\n", cont_t);
            motivo_final = MOT_BATBAJ1;
            mi_dron.modo = EST_FALLO;
            mostrar_estado();
            break;
        }

        if (mi_dron.modo == EST_RETORNO) {
            if (mi_dron.pos_f == base_f && mi_dron.pos_c == base_c) {
                mi_dron.modo = EST_INICIO;
                printf("[T:%d] Mision finalizada en base.\n", cont_t);
                if (verificar_todos()) motivo_final = MOT_EXITO;
                mostrar_estado();
                break;
            }
            mover_hacia_destino(base_f, base_c);
            mostrar_estado();
            mostrar_tablero();
            continue;
        }

        if (mi_clima.aviso == ST_PELIGRO) {
            printf("[T:%d] ALERTA PELIGRO CLIMA. Regreso forzado.\n", cont_t);
            mi_dron.modo = EST_RETORNO;
            motivo_final = MOT_CLIMAV;
            mostrar_estado();
            continue;
        }

        if (evaluar_sismo()) {
            printf("[T:%d] ¡REPLICA SISMICA DETECTADA!\n", cont_t);
            if (mi_dron.idx_actual >= 0 && mi_dron.pos_f == vector_obj[mi_dron.idx_actual].pos_f &&
                mi_dron.pos_c == vector_obj[mi_dron.idx_actual].pos_c && vector_obj[mi_dron.idx_actual].colapso) {
                printf("Zona de colapso afectada. Mision abortada.\n");
                motivo_final = MOT_SISMOC;
                mi_dron.modo = EST_FALLO;
                mostrar_estado();
                break;
            }
            mi_dron.modo = EST_RETORNO;
            if (motivo_final == MOT_NADA) motivo_final = MOT_SISMOT;
            mostrar_estado();
            continue;
        }

        if (mi_dron.idx_actual == -1) mi_dron.idx_actual = buscar_siguiente();

        if (verificar_todos()) {
            mi_dron.modo = EST_RETORNO;
            motivo_final = MOT_EXITO;
            continue;
        }

        {
            int obj_f = vector_obj[mi_dron.idx_actual].pos_f;
            int obj_c = vector_obj[mi_dron.idx_actual].pos_c;

            if (mi_dron.pos_f == obj_f && mi_dron.pos_c == obj_c) {
                printf("[T:%d] Objetivo alcanzado: %s\n", cont_t, vector_obj[mi_dron.idx_actual].etiqueta);
                vector_obj[mi_dron.idx_actual].listo = 1;
                metas_ok++;
                paquete_entregado = 1;
                mi_dron.peso_kg = 0.0;

                {
                    int sg = buscar_siguiente();
                    if (sg != -1) {
                        float bat_necesaria = (float)(calcular_distancia(mi_dron.pos_f, mi_dron.pos_c, vector_obj[sg].pos_f, vector_obj[sg].pos_c) +
                                              calcular_distancia(vector_obj[sg].pos_f, vector_obj[sg].pos_c, base_f, base_c)) * calcular_gasto() * 1.3;
                        if (mi_dron.nivel_bat > bat_necesaria) {
                            mi_dron.idx_actual = sg;
                        } else if (mi_dron.nivel_bat > bat_para_regresar()) {
                            mi_dron.modo = EST_RETORNO;
                            mi_dron.idx_actual = -1;
                        } else {
                            motivo_final = MOT_BATBAJ2;
                            mi_dron.modo = EST_FALLO;
                            break;
                        }
                    } else if (mi_dron.nivel_bat > bat_para_regresar()) {
                        mi_dron.modo = EST_RETORNO;
                        mi_dron.idx_actual = -1;
                    } else {
                        motivo_final = MOT_BATBAJ2;
                        mi_dron.modo = EST_FALLO;
                        break;
                    }
                }
                continue;
            }

            if (!mover_hacia_destino(obj_f, obj_c)) {
                if (mi_dron.intentos >= MAX_BLOQ) {
                    mi_dron.modo = EST_RETORNO;
                    mi_dron.intentos = 0;
                    if (motivo_final == MOT_NADA) motivo_final = MOT_TRABADO;
                }
            }
        }

        if (mi_dron.modo == EST_CURSO && mi_dron.nivel_bat <= bat_para_regresar()) {
            mi_dron.modo = EST_RETORNO;
            if (motivo_final == MOT_NADA) motivo_final = MOT_BATBAJ2;
        }

        mostrar_estado();
        mostrar_tablero();
    }

    printf("RESUMEN FINAL\n");
    printf("\n");
    printf("Causa: "); mostrar_motivo(motivo_final); printf("\n");
    printf("Carga entregada:     %s\n", paquete_entregado ? "SI (3 kg)" : "NO");
    printf("Objetivos atendidos: %d/%d\n", metas_ok, TOTAL_OBJ);
    printf("Turnos totales:      %d\n", cont_t);
    printf("Bateria restante:    %.0f%%\n", mi_dron.nivel_bat);
    printf("\n");
}

int main(void) {
    int valor_ingresado;
    printf("Ingrese semilla de simulacion: ");
    scanf("%d", &valor_ingresado);
    seed_gen = valor_ingresado;

    configurar_tablero();
    configurar_objetivos();
    configurar_dron();
    configurar_clima();
    ejecutar_simulacion();

    return 0;
}

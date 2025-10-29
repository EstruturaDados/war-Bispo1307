#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NAME 64
#define MAX_COLOR 32

/* ---------- Declarações de funções auxiliares ---------- */
void flush_input(void);

/* ---------- Estruturas ---------- */

typedef struct {
    char nome[MAX_NAME];
    char cor[MAX_COLOR]; // dono/cor do território
    int tropas;
} Territorio;

typedef enum {
    MISSAO_CONQUISTAR_X,   // possuir X territórios
    MISSAO_DESTRUIR_COR    // eliminar uma cor (nenhum território com essa cor)
} TipoMissao;

typedef struct {
    TipoMissao tipo;
    int param;              // para CONQUISTAR_X: número X; para DESTRUIR_COR: não usado
    char alvo_cor[MAX_COLOR]; // para DESTRUIR_COR: cor alvo
    char descricao[128];
} Missao;

/* ---------- Funções utilitárias ---------- */

void flush_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* usar nomes únicos para evitar conflitos com macros da plataforma */
static inline int my_min(int a, int b) { return (a < b) ? a : b; }
static inline int my_max(int a, int b) { return (a > b) ? a : b; }

/* ---------- Cadastro dinâmico de territórios ---------- */

Territorio* criar_vetor_territorios(int inicial) {
    Territorio *v = (Territorio*) malloc(sizeof(Territorio) * inicial);
    if (!v) {
        fprintf(stderr, "Erro de memória\n");
        exit(1);
    }
    return v;
}

/* ---------- Cadastro dinâmico de missões ---------- */

Missao* criar_vetor_missoes(int inicial) {
    Missao *v = (Missao*) malloc(sizeof(Missao) * inicial);
    if (!v) {
        fprintf(stderr, "Erro de memória (missões)\n");
        exit(1);
    }
    return v;
}

/* ---------- Listagem ---------- */

void listar_territorios(Territorio *t, int n) {
    if (n == 0) {
        printf("Nenhum território cadastrado.\n");
        return;
    }
    printf("ID  | Nome                          | Cor/Proprietário  | Tropas\n");
    printf("----+-------------------------------+-------------------+--------\n");
    for (int i = 0; i < n; ++i) {
        printf("%-3d | %-29s | %-17s | %-6d\n", i, t[i].nome, t[i].cor, t[i].tropas);
    }
}

/* ---------- Simulação de dados (ataque) ---------- */

int cmp_desc(const void *a, const void *b) {
    return (*(int*)b) - (*(int*)a);
}

void rolar_dados(int *dados, int n) {
    for (int i = 0; i < n; ++i) {
        dados[i] = (rand() % 6) + 1;
    }
    qsort(dados, n, sizeof(int), cmp_desc);
}

void ataque(Territorio *atacante, Territorio *defensor) {
    if (atacante->tropas < 2) {
        printf("Ataque impossível: o território atacante precisa ter pelo menos 2 tropas.\n");
        return;
    }
    if (strcmp(atacante->cor, defensor->cor) == 0) {
        printf("Ataque inválido: ambos os territórios pertencem ao mesmo jogador (%s).\n", atacante->cor);
        return;
    }

    int maxAtq = my_max(1, my_min(3, atacante->tropas - 1));
    int maxDef = my_max(1, my_min(2, defensor->tropas));

    printf("Tropas (Atacante %s): %d  |  Tropas (Defensor %s): %d\n",
           atacante->nome, atacante->tropas, defensor->nome, defensor->tropas);

    int atqDice = 0, defDice = 0;

    printf("Quantos dados o atacante usará? (1-%d): ", maxAtq);
    if (scanf("%d", &atqDice) != 1) { flush_input(); printf("Entrada inválida.\n"); return; }
    if (atqDice < 1) atqDice = 1;
    if (atqDice > maxAtq) atqDice = maxAtq;

    printf("Quantos dados o defensor usará? (1-%d): ", maxDef);
    if (scanf("%d", &defDice) != 1) { flush_input(); printf("Entrada inválida.\n"); return; }
    if (defDice < 1) defDice = 1;
    if (defDice > maxDef) defDice = maxDef;

    int dadosA[3] = {0}, dadosD[2] = {0};
    rolar_dados(dadosA, atqDice);
    rolar_dados(dadosD, defDice);

    printf("Dados Atacante: ");
    for (int i = 0; i < atqDice; ++i) printf("%d ", dadosA[i]);
    printf("\nDados Defensor: ");
    for (int i = 0; i < defDice; ++i) printf("%d ", dadosD[i]);
    printf("\n");

    /* usar my_min aqui também para consistência */
    int pares = my_min(atqDice, defDice);
    for (int i = 0; i < pares; ++i) {
        if (dadosA[i] > dadosD[i]) {
            defensor->tropas -= 1;
            printf("Atacante vence confronto %d: defensor perde 1 tropa.\n", i+1);
        } else {
            atacante->tropas -= 1;
            printf("Defensor vence confronto %d: atacante perde 1 tropa.\n", i+1);
        }
    }

    if (defensor->tropas <= 0) {
        printf("Território %s conquistado!\n", defensor->nome);
        int maxMover = atacante->tropas - 1;
        if (maxMover < 1) maxMover = 1;
        int mover;
        printf("Quantas tropas mover para o território conquistado? (1-%d): ", maxMover);
        if (scanf("%d", &mover) != 1) { flush_input(); mover = 1; }
        if (mover < 1) mover = 1;
        if (mover > maxMover) mover = maxMover;
        atacante->tropas -= mover;
        defensor->tropas = mover;
        strncpy(defensor->cor, atacante->cor, MAX_COLOR-1);
        defensor->cor[MAX_COLOR-1] = '\0';
        printf("Movidos %d tropas. %s agora pertence a %s.\n",
               mover, defensor->nome, defensor->cor);
    } else {
        printf("Batalha encerrada. Tropas agora: atacante=%d, defensor=%d\n",
               atacante->tropas, defensor->tropas);
    }
}

/* ---------- Sistema de missões ---------- */

void adicionar_missao(Missao **missoes, int *count, int *cap) {
    if (*count >= *cap) {
        *cap *= 2;
        *missoes = (Missao*) realloc(*missoes, sizeof(Missao) * (*cap));
        if (!(*missoes)) { fprintf(stderr, "Erro realloc (missões)\n"); exit(1); }
    }

    Missao m;
    printf("Tipo de missão:\n1 - Conquistar X territórios\n2 - Destruir cor\nEscolha: ");
    int tipo;
    if (scanf("%d", &tipo) != 1) { flush_input(); printf("Entrada inválida.\n"); return; }

    if (tipo == 1) {
        m.tipo = MISSAO_CONQUISTAR_X;
        printf("Quantos territórios precisa possuir para vencer? ");
        scanf("%d", &m.param);
        snprintf(m.descricao, sizeof(m.descricao), "Possuir %d territórios", m.param);
        m.alvo_cor[0] = '\0';
    } else {
        m.tipo = MISSAO_DESTRUIR_COR;
        printf("Informe a cor/jogador a ser destruído: ");
        flush_input();
        fgets(m.alvo_cor, MAX_COLOR, stdin);
        m.alvo_cor[strcspn(m.alvo_cor, "\n")] = '\0';
        snprintf(m.descricao, sizeof(m.descricao), "Eliminar jogador/cor %s", m.alvo_cor);
        m.param = 0;
    }

    (*missoes)[*count] = m;
    (*count)++;
    printf("Missão adicionada: %s\n", m.descricao);
}

int checar_missao(Missao *m, Territorio *territorios, int n) {
    if (m->tipo == MISSAO_CONQUISTAR_X) {
        for (int i = 0; i < n; ++i) {
            if (strlen(territorios[i].cor) == 0) continue;
            int count = 0;
            for (int j = 0; j < n; ++j)
                if (strcmp(territorios[i].cor, territorios[j].cor) == 0)
                    count++;
            if (count >= m->param) return 1;
        }
        return 0;
    } else if (m->tipo == MISSAO_DESTRUIR_COR) {
        for (int i = 0; i < n; ++i)
            if (strcmp(territorios[i].cor, m->alvo_cor) == 0)
                return 0;
        return 1;
    }
    return 0;
}

void listar_missoes(Missao *missoes, int n) {
    if (n == 0) {
        printf("Nenhuma missão cadastrada.\n");
        return;
    }
    for (int i = 0; i < n; ++i)
        printf("%d) %s\n", i, missoes[i].descricao);
}

/* ---------- Programa principal ---------- */

int main(void) {
    srand((unsigned) time(NULL));

    int capT = 4, nT = 0;
    Territorio *territorios = criar_vetor_territorios(capT);

    int capM = 2, nM = 0;
    Missao *missoes = criar_vetor_missoes(capM);

    int opc = 0;
    while (1) {
        printf("\n--- MENU ---\n");
        printf("1 - Cadastrar território\n");
        printf("2 - Listar territórios\n");
        printf("3 - Simular ataque\n");
        printf("4 - Adicionar missão\n");
        printf("5 - Listar missões\n");
        printf("6 - Checar missões (verificar vitória)\n");
        printf("7 - Remover território\n");
        printf("0 - Sair\n");
        printf("Escolha: ");

        if (scanf("%d", &opc) != 1) { flush_input(); printf("Entrada inválida.\n"); continue; }

        if (opc == 0) break;
        else if (opc == 1) {
            flush_input();
            if (nT >= capT) {
                capT *= 2;
                territorios = realloc(territorios, sizeof(Territorio) * capT);
                if (!territorios) { fprintf(stderr, "Erro realloc (territórios)\n"); exit(1); }
            }
            printf("Nome do território: ");
            fgets(territorios[nT].nome, MAX_NAME, stdin);
            territorios[nT].nome[strcspn(territorios[nT].nome, "\n")] = '\0';
            printf("Cor/Proprietário: ");
            fgets(territorios[nT].cor, MAX_COLOR, stdin);
            territorios[nT].cor[strcspn(territorios[nT].cor, "\n")] = '\0';
            printf("Número de tropas: ");
            if (scanf("%d", &territorios[nT].tropas) != 1) { flush_input(); territorios[nT].tropas = 1; }
            nT++;
            printf("Território cadastrado com ID %d\n", nT-1);
        } 
        else if (opc == 2) listar_territorios(territorios, nT);
        else if (opc == 3) {
            if (nT < 2) { printf("Necessários pelo menos 2 territórios.\n"); continue; }
            listar_territorios(territorios, nT);
            printf("ID atacante: "); int idA; scanf("%d", &idA);
            printf("ID defensor: "); int idD; scanf("%d", &idD);
            if (idA < 0 || idA >= nT || idD < 0 || idD >= nT || idA == idD) {
                printf("IDs inválidos.\n"); continue;
            }
            ataque(&territorios[idA], &territorios[idD]);
        } 
        else if (opc == 4) adicionar_missao(&missoes, &nM, &capM);
        else if (opc == 5) listar_missoes(missoes, nM);
        else if (opc == 6) {
            if (nM == 0) { printf("Nenhuma missão cadastrada.\n"); continue; }
            for (int i = 0; i < nM; ++i) {
                int ok = checar_missao(&missoes[i], territorios, nT);
                printf("Missão %d (%s): %s\n", i, missoes[i].descricao, ok ? "COMPLETA" : "NÃO COMPLETA");
            }
        } 
        else if (opc == 7) {
            listar_territorios(territorios, nT);
            printf("ID para remover: ");
            int id; scanf("%d", &id);
            if (id < 0 || id >= nT) { printf("ID inválido.\n"); continue; }
            territorios[id] = territorios[nT - 1];
            nT--;
            printf("Território removido.\n");
        } 
        else printf("Opção inválida.\n");
    }

    free(territorios);
    free(missoes);
    printf("Encerrando...\n");
    return 0;
}

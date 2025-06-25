#include "aluno.h"
#include "curso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h> // Para trunc

// --- Implementação das Funções de Manipulação de TFunc ---

// Retorna o tamanho em bytes de um registro TFunc
int tamanho_registro() {
    return sizeof(TFunc);
}

// Lê um registro de funcionário do arquivo
TFunc *le(FILE *in) {
    TFunc *func = (TFunc *) malloc(sizeof(TFunc));
    if (func == NULL) {
        perror("Erro ao alocar memoria para funcionario");
        exit(1);
    }
    // fread retorna o número de itens lidos com sucesso.
    // Se for menor que 1, significa que chegou ao fim do arquivo ou houve um erro.
    if (fread(func, tamanho_registro(), 1, in) < 1) {
        free(func); // Libera a memória alocada se a leitura falhar
        return NULL;
    }
    return func;
}

// Salva um registro de funcionário no arquivo
void salva(TFunc *func, FILE *out) {
    fwrite(func, tamanho_registro(), 1, out);
}


// =================================================================================
//   FUNÇÃO DE BUSCA BINÁRIA CORRIGIDA
// =================================================================================
TFunc *busca_binaria(int chave, FILE *in, int inicio, int fim, FILE *log) {
    TFunc *f = NULL;
    int cont = 0;
    clock_t inicioT, fimT;
    double total;

    inicioT = clock();

    /*
     * CORREÇÃO PRINCIPAL:
     * O laço deve continuar apenas enquanto o espaço de busca for válido (inicio <= fim).
     * A verificação da chave é feita DENTRO do laço.
    */
    while (inicio <= fim) {
        cont++; // Incrementa o contador de comparações a cada iteração

        // Cálculo mais seguro para o meio, evita overflow com números grandes
        int meio = inicio + (fim - inicio) / 2;

        // Posiciona o ponteiro do arquivo no registro do meio
        // Se os registros são 0-indexados, a posição é 'meio'.
        fseek(in, meio * tamanho_registro(), SEEK_SET);

        // Lê o registro
        f = le(in);
        if (f == NULL) {
            // Se a leitura falhar (improvável no meio do arquivo, mas é uma segurança), para a busca.
            break;
        }

        if (f->cod == chave) {
            // --- ENCONTRADO ---
            fprintf(log, "\nBusca Binaria - Comparações: %d", cont);
            fimT = clock();
            total = ((double)(fimT - inicioT)) / CLOCKS_PER_SEC;
            fprintf(log, "\nBusca Binaria - Tempo: %f segundos", total);
            return f; // Retorna o ponteiro para o funcionário encontrado
        }
        else if (f->cod < chave) {
            // Se a chave for maior, busca na metade direita
            inicio = meio + 1;
        }
        else { // f->cod > chave
            // Se a chave for menor, busca na metade esquerda
            fim = meio - 1;
        }
        
        /*
         * CORREÇÃO DE VAZAMENTO DE MEMÓRIA:
         * Como o registro lido não era o correto, liberamos sua memória
         * antes de continuar para a próxima iteração.
        */
        free(f);
        f = NULL;
    }

    // Se o laço terminar, o funcionário não foi encontrado
    printf("Funcionario com código %d não encontrado na busca binária.\n", chave);
    fimT = clock();
    total = ((double)(fimT - inicioT)) / CLOCKS_PER_SEC;
    fprintf(log, "\nBusca Binaria (Não encontrado) - Comparações: %d", cont);
    fprintf(log, "\nBusca Binaria (Não encontrado) - Tempo: %f segundos", total);
    return NULL; // Retorna NULL para indicar que não encontrou
}


// =================================================================================
//   FUNÇÃO DE BUSCA SEQUENCIAL CORRIGIDA
// =================================================================================
TFunc *buscaSequencial(int chave, FILE *in, FILE *log) {
    TFunc *f = NULL;
    int achou = 0; // CORREÇÃO: Inicializar a variável 'achou'
    int cont = 0;
    clock_t inicio_t, fim_t;
    double total_t;

    rewind(in); // Garante que a busca comece do início do arquivo
    inicio_t = clock();

    while ((f = le(in)) != NULL) {
        cont++;
        if (f->cod == chave) {
            achou = 1;
            break; // Para o laço assim que encontrar
        }
        /*
         * Libera a memória do registro lido se não for o que procuramos.
         * Isso evita vazamento de memória no caso de não encontrar.
         */
        free(f);
    }

    fim_t = clock();
    total_t = ((double)(fim_t - inicio_t)) / CLOCKS_PER_SEC;

    if (achou == 1) {
        fprintf(log, "\nBusca Sequencial - Comparações: %d", cont);
        fprintf(log, "\nBusca Sequencial - Tempo: %f segundos", total_t);
        return f; // Retorna o ponteiro para o funcionário encontrado
    } else {
        printf("Funcionario com código %d não encontrado na busca sequencial.\n", chave);
        fprintf(log, "\nBusca Sequencial (Não encontrado) - Comparações: %d", cont);
        // CORREÇÃO: Usar a variável de tempo (total_t) e o formato correto (%f)
        fprintf(log, "\nBusca Sequencial (Não encontrado) - Tempo: %f segundos", total_t);
        return NULL;
    }
    /*
     * CORREÇÃO CRÍTICA:
     * O 'free(f)' foi removido daqui. Se 'achou' fosse 1, 'f' já teria sido retornado,
     * e liberá-lo aqui corromperia a memória para o chamador (use-after-free).
     * Se 'achou' fosse 0, 'f' seria NULL no final do laço, e free(NULL) é seguro mas desnecessário.
     * A memória agora é liberada dentro do loop.
    */
}


// --- FUNÇÃO MAIN PARA TESTE ---

int main() {
    // 1. Criar um arquivo de teste com registros ordenados por código
    FILE *out = fopen("funcionarios.dat", "wb");
    if (out == NULL) {
        perror("Erro ao abrir arquivo para escrita.");
        return 1;
    }

    printf("Criando arquivo de teste 'funcionarios.dat'...\n");
    for (int i = 1; i <= 100; i++) {
        TFunc f = {i, "Funcionario", "111.111.111-11", "01/01/2000", 1500.0 + i};
        sprintf(f.nome, "Funcionario %03d", i); // Formata o nome para ser único
        salva(&f, out);
    }
    fclose(out);
    printf("Arquivo criado com 100 funcionários (código de 1 a 100).\n\n");

    // 2. Abrir os arquivos para leitura e para log
    FILE *in = fopen("funcionarios.dat", "rb");
    FILE *log = fopen("log_buscas.txt", "a"); // 'a' para adicionar ao final do log
    if (in == NULL || log == NULL) {
        perror("Erro ao abrir arquivo para leitura/log.");
        return 1;
    }
    
    TFunc *resultado = NULL;
    int total_registros = 100;
    int chave_para_buscar = 78; // Chave que existe

    // Testando a busca sequencial
    printf("--- Testando Busca Sequencial (chave = %d) ---\n", chave_para_buscar);
    resultado = buscaSequencial(chave_para_buscar, in, log);
    if (resultado != NULL) {
        printf("Encontrado: Cod: %d, Nome: %s\n", resultado->cod, resultado->nome);
        free(resultado); // A função que chama a busca é responsável por liberar a memória!
    }

    printf("\n--- Testando Busca Binária (chave = %d) ---\n", chave_para_buscar);
    // Para a binária, precisamos saber o número de registros (ou calcular)
    resultado = busca_binaria(chave_para_buscar, in, 0, total_registros - 1, log);
    if (resultado != NULL) {
        printf("Encontrado: Cod: %d, Nome: %s\n", resultado->cod, resultado->nome);
        free(resultado); // Liberar a memória após o uso
    }
    
    // Testando um caso de falha
    chave_para_buscar = 200;
     printf("\n--- Testando com chave inexistente (chave = %d) ---\n", chave_para_buscar);
    resultado = buscaSequencial(chave_para_buscar, in, log); // Já imprime a mensagem de erro
    resultado = busca_binaria(chave_para_buscar, in, 0, total_registros - 1, log); // Já imprime a mensagem de erro


    fclose(in);
    fclose(log);
    printf("\nTestes concluídos. Verifique o arquivo 'log_buscas.txt'.\n");

    return 0;
}
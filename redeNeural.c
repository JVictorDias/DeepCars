#include <gsl/gsl_rng.h>
#include <math.h>
#include <time.h>

#define TAXA_APRENDIZADO    (0.1)
#define TAXA_PESO_INICIAL   (1.0)
#define BIAS                1

//#define Ativacao(X)         (1.0/(1.0+exp(-X)))
//#define Derivada(X)         (X*(1.0-X))

//#define Ativacao(X)         tanh(X)
//#define Derivada(X)         (1.0 - (tanh(X)*tanh(X)))

//#define Ativacao(X)         func(X)

//#define Ativacao(X)         relu(X)
//#define Derivada(X)         reluDx(X)


#define AtivacaoOcultas(X)        relu(X)
#define AtivacaoSaida(X)          relu(X)

typedef struct neuronio
{
    double* Peso;
    double  Erro;
    double  Saida;

    int QuantidadeLigacoes;

}   Neuronio;

typedef struct camada
{
    Neuronio* Neuronios;

    int QuantidadeNeuronios;

}   Camada;

typedef struct redeNeural
{
    Camada  CamadaEntrada;
    Camada* CamadaEscondida;
    Camada  CamadaSaida;

    int QuantidadeEscondidas;

}   RedeNeural;

    gsl_rng*        Gerador;

double func(double X)
{
    if(X == 0)
    {
        return 0;
    }
    if(X < 0)
    {
        return -1;
    }
    if(X > 0)
    {
        return 1;
    }
}

double relu(double X)
{
    if(X < 0)
    {
        return 0;
    }
    else
    {
        if(X < 10000)
        {
            return X;
        }
        else
        {
            return 10000;
        }
    }
}

double reluDx(double X)
{
    if(X < 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void RNA_CopiarVetorParaCamadas(RedeNeural* Rede, double* Vetor)
{
    int j,k,l;

    j = 0;

    for(int i=0; i<Rede->QuantidadeEscondidas; i++)
    {
        for(k=0; k<Rede->CamadaEscondida[i].QuantidadeNeuronios; k++)
        {
            for(l=0; l<Rede->CamadaEscondida[i].Neuronios[k].QuantidadeLigacoes; l++)
            {
                Rede->CamadaEscondida[i].Neuronios[k].Peso[l] = Vetor[j];
                j++;
            }
        }
    }

    /////////////////////
    for(k=0; k<Rede->CamadaSaida.QuantidadeNeuronios; k++)
    {
        for(l=0; l<Rede->CamadaSaida.Neuronios[k].QuantidadeLigacoes; l++)
        {
            Rede->CamadaSaida.Neuronios[k].Peso[l] = Vetor[j];
            j++;
        }
    }
}

void RNA_CopiarCamadasParaVetor(RedeNeural* Rede, double* Vetor)
{
    int j,k,l;

    j = 0;

    for(int i=0; i<Rede->QuantidadeEscondidas; i++)
    {
        for(k=0; k<Rede->CamadaEscondida[i].QuantidadeNeuronios; k++)
        {
            for(l=0; l<Rede->CamadaEscondida[i].Neuronios[k].QuantidadeLigacoes; l++)
            {
                Vetor[j] = Rede->CamadaEscondida[i].Neuronios[k].Peso[l];
                j++;
            }
        }
    }

    /////////////////////
    for(k=0; k<Rede->CamadaSaida.QuantidadeNeuronios; k++)
    {
        for(l=0; l<Rede->CamadaSaida.Neuronios[k].QuantidadeLigacoes; l++)
        {
            Vetor[j] = Rede->CamadaSaida.Neuronios[k].Peso[l];
            j++;
        }
    }
}



void RNA_CopiarParaEntrada(RedeNeural* Rede, double* VetorEntrada)
{
    int i;

    for(i=0; i<Rede->CamadaEntrada.QuantidadeNeuronios - BIAS; i++)
    {
        Rede->CamadaEntrada.Neuronios[i].Saida = VetorEntrada[i];
    }
}

int RNA_QuantidadePesos(RedeNeural* Rede)
{
    int Soma = 0;
    for(int i=0; i<Rede->QuantidadeEscondidas; i++)
    {
        for(int j=0; j<Rede->CamadaEscondida[i].QuantidadeNeuronios; j++)
        {
            Soma = Soma + Rede->CamadaEscondida[i].Neuronios[j].QuantidadeLigacoes;
        }
    }

    for(int i=0; i<Rede->CamadaSaida.QuantidadeNeuronios; i++)
    {
        Soma = Soma + Rede->CamadaSaida.Neuronios[i].QuantidadeLigacoes;
    }
    return Soma;
}

void RNA_CopiarDaSaida(RedeNeural* Rede, double* VetorSaida)
{
    int i;

    for(i=0; i<Rede->CamadaSaida.QuantidadeNeuronios; i++)
    {
        VetorSaida[i] = Rede->CamadaSaida.Neuronios[i].Saida;
    }
}

void RNA_CalcularSaida(RedeNeural* Rede)
{
    int i,j,k;
    double Somatorio;

    /// Calculando saidas entre a camada de entrada e a primeira camada escondida ///////////////////////////////////////////////////////////////////////////////
    for(i=0; i<Rede->CamadaEscondida[0].QuantidadeNeuronios - BIAS; i++)
    {
        Somatorio = 0;
        for(j=0; j<Rede->CamadaEntrada.QuantidadeNeuronios; j++)
        {
            Somatorio = Somatorio + Rede->CamadaEntrada.Neuronios[j].Saida * Rede->CamadaEscondida[0].Neuronios[i].Peso[j];
        }
        Rede->CamadaEscondida[0].Neuronios[i].Saida = AtivacaoOcultas(Somatorio);
    }
    //////////////////////////////////////////////////////////////////////////////////
    /// Calculando saidas entre a camada escondida k e a camada escondida k-1 ///////////////////////////////////////////////////////////////////////////////

    for(k=1; k<Rede->QuantidadeEscondidas; k++)
    {
        for(i=0; i<Rede->CamadaEscondida[k].QuantidadeNeuronios - BIAS; i++)
        {
            Somatorio = 0;
            for(j=0; j<Rede->CamadaEscondida[k-1].QuantidadeNeuronios; j++)
            {
                Somatorio = Somatorio + Rede->CamadaEscondida[k-1].Neuronios[j].Saida * Rede->CamadaEscondida[k].Neuronios[i].Peso[j];
            }
            Rede->CamadaEscondida[k].Neuronios[i].Saida = AtivacaoOcultas(Somatorio);
        }
    }
    //////////////////////////////////////////////////////////////////////////////////
    /// Calculando saidas entre a camada de saida e a ultima camada escondida ///////////////////////////////////////////////////////////////////////////////
    for(i=0; i<Rede->CamadaSaida.QuantidadeNeuronios; i++)
    {
        Somatorio = 0;
        for(j=0; j<Rede->CamadaEscondida[k-1].QuantidadeNeuronios; j++)
        {
            Somatorio = Somatorio + Rede->CamadaEscondida[k-1].Neuronios[j].Saida * Rede->CamadaSaida.Neuronios[i].Peso[j];
        }
        Rede->CamadaSaida.Neuronios[i].Saida = AtivacaoSaida(Somatorio);
    }
}

//int RNA_BackPropagation(RedeNeural* Rede, double* Entrada, double* SaidaEsperada)
//{
//    int i,j,k;
//    double Somatorio;
//    int Erro;
//
//    RNA_CopiarParaEntrada(Rede, Entrada);
//    RNA_CalcularSaida(Rede);
//
//    double Saida[DINO_BRAIN_QTD_OUTPUT];
//    for(int i=0; i<DINO_BRAIN_QTD_OUTPUT; i++)
//    {
//        Saida[i] = Rede->CamadaSaida.Neuronios[i].Saida;
//    }
////    Erro = CompararOutput(Entrada, Saida, SaidaEsperada);
//
//    if(Erro)
//    {
//        /// Setando erro da camada de saida ///////////////////////
//
//        for(i=0; i<Rede->CamadaSaida.QuantidadeNeuronios; i++)
//        {
//            Rede->CamadaSaida.Neuronios[i].Erro = SaidaEsperada[i] - Rede->CamadaSaida.Neuronios[i].Saida;
//            //Erro = fabs(Rede->CamadaSaida.Neuronios[i].Erro);
//        }
//
//        //////////////////////////////////////////////////////////////////
//        /// Atualizando erro entre ultima camada escondida e camada saida ///////////////////////
//
//        for(j=0; j<Rede->CamadaEscondida[Rede->QuantidadeEscondidas-1].QuantidadeNeuronios; j++)
//        {
//            Somatorio = 0;
//            for(k=0; k<Rede->CamadaSaida.QuantidadeNeuronios; k++)
//            {
//                Somatorio = Somatorio + Rede->CamadaSaida.Neuronios[k].Erro * Rede->CamadaSaida.Neuronios[k].Peso[j];
//            }
//            Rede->CamadaEscondida[Rede->QuantidadeEscondidas-1].Neuronios[j].Erro = Somatorio;
//        }
//
//        //////////////////////////////////////////////////////////////////
//        /// Atualizando erro entre a camada escondida 'n' e 'n+1' ///////////////////////
//
//        for(i=Rede->QuantidadeEscondidas-2; i>=0; i--)
//        {
//            for(j=0; j<Rede->CamadaEscondida[i].QuantidadeNeuronios; j++)
//            {
//                Somatorio = 0;
//                for(k=0; k<Rede->CamadaEscondida[i+1].QuantidadeNeuronios; k++)
//                {
//                    Somatorio = Somatorio + Rede->CamadaEscondida[i+1].Neuronios[k].Erro * Rede->CamadaEscondida[i+1].Neuronios[k].Peso[j];
//                }
//                Rede->CamadaEscondida[i].Neuronios[j].Erro = Somatorio;
//            }
//        }
//        /// Atualizando pesos camada entrada com primeira escondida////////////////////////
//
//        for(j=0; j<Rede->CamadaEscondida[0].QuantidadeNeuronios; j++)
//        {
//            for(k=0; k<Rede->CamadaEntrada.QuantidadeNeuronios; k++)
//            {
//                Rede->CamadaEscondida[0].Neuronios[j].Peso[k] = Rede->CamadaEscondida[0].Neuronios[j].Peso[k] + (TAXA_APRENDIZADO*
//                                                                                                                 Rede->CamadaEscondida[0].Neuronios[j].Erro*
//                                                                                                                 Rede->CamadaEntrada.Neuronios[k].Saida*
//                                                                                                                 Derivada(Rede->CamadaEscondida[0].Neuronios[j].Saida));
//            }
//        }
//
//        /// Atualizando pesos camada 'n' com 'n-1'////////////////////////
//        for(i=1; i<Rede->QuantidadeEscondidas; i++)
//        {
//            for(j=0; j<Rede->CamadaEscondida[i].QuantidadeNeuronios; j++)
//            {
//                for(k=0; k<Rede->CamadaEscondida[i-1].QuantidadeNeuronios; k++)
//                {
//                    Rede->CamadaEscondida[i].Neuronios[j].Peso[k] = Rede->CamadaEscondida[i].Neuronios[j].Peso[k] + (TAXA_APRENDIZADO*
//                                                                                                                     Rede->CamadaEscondida[i].Neuronios[j].Erro*
//                                                                                                                     Rede->CamadaEscondida[i-1].Neuronios[k].Saida*
//                                                                                                                     Derivada(Rede->CamadaEscondida[i].Neuronios[j].Saida));
//                }
//            }
//        }
//        /// Atualizando pesos camada sai com ultima escondida ////////////////////////
//        for(j=0; j<Rede->CamadaSaida.QuantidadeNeuronios; j++)
//        {
//            for(k=0; k<Rede->CamadaEscondida[Rede->QuantidadeEscondidas-1].QuantidadeNeuronios; k++)
//            {
//                Rede->CamadaSaida.Neuronios[j].Peso[k] = Rede->CamadaSaida.Neuronios[j].Peso[k] + (TAXA_APRENDIZADO*
//                                                                                                   Rede->CamadaSaida.Neuronios[j].Erro*
//                                                                                                   Rede->CamadaEscondida[Rede->QuantidadeEscondidas-1].Neuronios[k].Saida*
//                                                                                                   Derivada(Rede->CamadaSaida.Neuronios[j].Saida));
//            }
//        }
//
//    }
//
//    return 0;
//    //return Erro;
//}

void RNA_CriarNeuronio(Neuronio* Neuron, int QuantidadeLigacoes)
{
    int i;

    Neuron->QuantidadeLigacoes = QuantidadeLigacoes;
    Neuron->Peso = (double*)malloc(QuantidadeLigacoes*sizeof(double));

    for(i=0; i<QuantidadeLigacoes; i++)
    {
        if(gsl_rng_uniform_int(Gerador,2) == 0)
        {
            Neuron->Peso[i] = gsl_rng_uniform(Gerador)/TAXA_PESO_INICIAL;
        }
        else
        {
            Neuron->Peso[i] = -gsl_rng_uniform(Gerador)/TAXA_PESO_INICIAL;
        }
    }

    Neuron->Erro = 0;
    Neuron->Saida = 1;
}

RedeNeural* RNA_CriarRedeNeural(int QuantidadeEscondidas, int QtdNeuroniosEntrada, int QtdNeuroniosEscondida, int QtdNeuroniosSaida)
{
    int i, j;

    QtdNeuroniosEntrada     = QtdNeuroniosEntrada + BIAS;
    QtdNeuroniosEscondida   = QtdNeuroniosEscondida + BIAS;

    RedeNeural* Rede = (RedeNeural*)malloc(sizeof(RedeNeural));

    Rede->CamadaEntrada.QuantidadeNeuronios = QtdNeuroniosEntrada;
    Rede->CamadaEntrada.Neuronios = (Neuronio*)malloc(QtdNeuroniosEntrada*sizeof(Neuronio));

    for(i=0; i<QtdNeuroniosEntrada; i++)
    {
        Rede->CamadaEntrada.Neuronios[i].Saida = 1.0;
    }

    Rede->QuantidadeEscondidas = QuantidadeEscondidas;
    Rede->CamadaEscondida = (Camada*)malloc(QuantidadeEscondidas*sizeof(Camada));

    for(i=0; i<QuantidadeEscondidas; i++)
    {
        Rede->CamadaEscondida[i].QuantidadeNeuronios = QtdNeuroniosEscondida;
        Rede->CamadaEscondida[i].Neuronios = (Neuronio*)malloc(QtdNeuroniosEscondida*sizeof(Neuronio));

        for(j=0; j<QtdNeuroniosEscondida; j++)
        {
            if(i == 0)
            {
                RNA_CriarNeuronio(&Rede->CamadaEscondida[i].Neuronios[j], QtdNeuroniosEntrada);
            }
            else
            {
                RNA_CriarNeuronio(&Rede->CamadaEscondida[i].Neuronios[j], QtdNeuroniosEscondida);
            }
        }
    }

    Rede->CamadaSaida.QuantidadeNeuronios = QtdNeuroniosSaida;
    Rede->CamadaSaida.Neuronios = (Neuronio*)malloc(QtdNeuroniosSaida*sizeof(Neuronio));

    for(j=0; j<QtdNeuroniosSaida; j++)
    {
        RNA_CriarNeuronio(&Rede->CamadaSaida.Neuronios[j], QtdNeuroniosEscondida);
    }

    //printf("Criada uma Rede Neural com:\n\n1 Camada de entrada com %d neuronio(s) + 1 BIAS.\n%d Camada(s) escondida(s), cada uma com %d neuronio(s) + 1 BIAS.\n1 Camada de saida com %d neuronio(s).\n", QtdNeuroniosEntrada-1, QuantidadeEscondidas, QtdNeuroniosEscondida-1, QtdNeuroniosSaida);

    return Rede;
}

RedeNeural* RNA_DestruirRedeNeural(RedeNeural* Rede)
{
    int i,j;

    free(Rede->CamadaEntrada.Neuronios);
    /////////////////////////////////////////////////////////////
    for(j=0; j<Rede->QuantidadeEscondidas; j++)
    {
        for(i=0; i<Rede->CamadaEscondida[j].QuantidadeNeuronios; i++)
        {
            free(Rede->CamadaEscondida[j].Neuronios[i].Peso);
        }
        free(Rede->CamadaEscondida[j].Neuronios);
    }
    free(Rede->CamadaEscondida);
    /////////////////////////////////////////////////////////
    for(i=0; i<Rede->CamadaSaida.QuantidadeNeuronios; i++)
    {
        free(Rede->CamadaSaida.Neuronios[i].Peso);
    }
    free(Rede->CamadaSaida.Neuronios);

    return NULL;
}

RedeNeural* RNA_CarregarRede(char* String)
{
    int i,j,k;
    FILE* f;
    RedeNeural* Temp;

    int QtdEscondida, QtdNeuroEntrada, QtdNeuroSaida, QtdNeuroEscondida;

    f = fopen(String,"rb");
    if(f != NULL)
    {
        fread(&QtdEscondida,1,sizeof(int),f);
        fread(&QtdNeuroEntrada,1,sizeof(int),f);
        fread(&QtdNeuroEscondida,1,sizeof(int),f);
        fread(&QtdNeuroSaida,1,sizeof(int),f);

        Temp = RNA_CriarRedeNeural(QtdEscondida,QtdNeuroEntrada,QtdNeuroEscondida,QtdNeuroSaida);

        for(k=0; k<Temp->QuantidadeEscondidas; k++)
        {
            for(i=0; i<Temp->CamadaEscondida[k].QuantidadeNeuronios; i++)
            {
                for(j=0; j<Temp->CamadaEscondida[k].Neuronios[i].QuantidadeLigacoes; j++)
                {
                    fread(&(Temp->CamadaEscondida[k].Neuronios[i].Peso[j]),1,8,f);
                }
            }
        }
        for(i=0; i<Temp->CamadaSaida.QuantidadeNeuronios; i++)
        {
            for(j=0; j<Temp->CamadaSaida.Neuronios[i].QuantidadeLigacoes; j++)
            {
                fread(&(Temp->CamadaSaida.Neuronios[i].Peso[j]),1,8,f);
            }
        }

        fclose(f);
        return Temp;
    }
}

void RNA_SalvarRede(RedeNeural* Temp, char* String)
{
    int i,j,k;
    FILE* f;

    f = fopen(String,"wb");
    if(f != NULL)
    {
        fwrite(&Temp->QuantidadeEscondidas,1,sizeof(int),f);
        fwrite(&Temp->CamadaEntrada.QuantidadeNeuronios,1,sizeof(int),f);
        fwrite(&Temp->CamadaEscondida[0].QuantidadeNeuronios,1,sizeof(int),f);
        fwrite(&Temp->CamadaSaida.QuantidadeNeuronios,1,sizeof(int),f);

        for(k=0; k<Temp->QuantidadeEscondidas; k++)
        {
            for(i=0; i<Temp->CamadaEscondida[k].QuantidadeNeuronios; i++)
            {
                for(j=0; j<Temp->CamadaEscondida[k].Neuronios[i].QuantidadeLigacoes; j++)
                {
                    fwrite(&Temp->CamadaEscondida[k].Neuronios[i].Peso[j],1,8,f);
                }
            }
        }

        for(i=0; i<Temp->CamadaSaida.QuantidadeNeuronios; i++)
        {
            for(j=0; j<Temp->CamadaSaida.Neuronios[i].QuantidadeLigacoes; j++)
            {
                fwrite(&Temp->CamadaSaida.Neuronios[i].Peso[j],1,8,f);
            }
        }

        fclose(f);
    }
}

void RNA_ImprimirPesos(RedeNeural* Temp)
{
    int i,j,k;
    FILE* f;

    for(k=0; k<Temp->QuantidadeEscondidas; k++)
    {
        printf("Camada escondida %d\n",k);
        for(i=0; i<Temp->CamadaEscondida[k].QuantidadeNeuronios; i++)
        {
            printf("\tNeuronio %d\n",i);
            for(j=0; j<Temp->CamadaEscondida[k].Neuronios[i].QuantidadeLigacoes; j++)
            {
                printf("\t\tPeso %d: %f\n", j, Temp->CamadaEscondida[k].Neuronios[i].Peso[j]);
            }
        }
    }

    printf("Camada saida %d\n",k);
    for(i=0; i<Temp->CamadaSaida.QuantidadeNeuronios; i++)
    {
        printf("\tNeuronio %d\n",i);
        for(j=0; j<Temp->CamadaSaida.Neuronios[i].QuantidadeLigacoes; j++)
        {
            printf("\t\tPeso %d: %f\n", j, Temp->CamadaSaida.Neuronios[i].Peso[j]);
        }
    }
}


void InicializarGeradorAleatorio()
{
    Gerador = gsl_rng_alloc(gsl_rng_ranlxs0);
    gsl_rng_set(Gerador,time(NULL));
}



//int main()
//{
//    Gerador = gsl_rng_alloc(gsl_rng_ranlxs0);
//    gsl_rng_set(Gerador,time(NULL));
//
//    Victor = RNA_CriarRedeNeural(1,     2, 10, 1);
//
//    double Entrada1[2] = {0,0};
//    double Entrada2[2] = {1,1};
//    double Entrada3[2] = {0,1};
//    double Entrada4[2] = {1,0};
//
//    double Saida1 = 0;
//    double Saida2 = 0;
//    double Saida3 = 1;
//    double Saida4 = 1;
//
//    for(int i=0; i<1000; i++)
//    {
//        printf("\n");
//        RNA_BackPropagation(Victor, Entrada1, &Saida1);
//        RNA_BackPropagation(Victor, Entrada2, &Saida2);
//        RNA_BackPropagation(Victor, Entrada3, &Saida3);
//        RNA_BackPropagation(Victor, Entrada4, &Saida4);
//    }
//
//    Victor = RNA_DestruirRedeNeural(Victor);
//
//    gsl_rng_free(Gerador);
//
//
//    return 0;
//}

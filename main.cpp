#include "PIG.h"
#include "redeNeural.c"

#define DESLOCAMENTO_NEURONIOS  85

#define LARGURA_CENARIO         2000
#define ALTURA_CENARIO          2000

#define QTD_CARROS              1000
#define QTD_SPRITES_CARROS      7
#define QTD_OBSTACULO           23
#define QTD_ZONA                8

#define MAX_EXPLOSOES           QTD_CARROS

#define CAR_BRAIN_QTD_LAYERS   1
#define CAR_BRAIN_QTD_INPUT    18
#define CAR_BRAIN_QTD_HIDE     6
#define CAR_BRAIN_QTD_OUTPUT   4

typedef struct carro
{
    double X, Y;
    double VX, VY;
    double Angulo;
    double Velocidade;
    char Colidiu;
    char Queimado;

    int Sprite;
    double DistanciaSensores[CAR_BRAIN_QTD_INPUT-1];

    int TamanhoDNA;
    double* DNA;
    double Fitness;

    RedeNeural* Cerebro;

}   Carro;

typedef struct animation2
{
    int X;
    int Y;
    int Largura;
    int Altura;
    double Angulo;

    int Ativada;
    int FrameAtual;
    int QuantidadeFrames;

}   Animation2;

typedef struct obstaculo
{
    double X,Y;
    double VX,VY;
    double angulo;
    double velocidadeRotacao;
    double tamanhoX, tamanhoY;
    double rangeMaxMovimento;
    double rangeAtualMovimento;
    int emMovimento;

}   Obstaculo;

typedef struct zona
{
    double valor;
    double angulo;
    double X,Y,larguraX,larguraY;

}   Zona;

    SDL_Point   paredes[LARGURA_CENARIO*ALTURA_CENARIO];
    SDL_Point   paredesVirtual[LARGURA_CENARIO*ALTURA_CENARIO];
    int         quantidadeParede;

    char        cenario[LARGURA_CENARIO][ALTURA_CENARIO];
    int         distancias[LARGURA_CENARIO][ALTURA_CENARIO];
    double      matrizBoost[LARGURA_CENARIO][ALTURA_CENARIO];

    int         spritePista, spriteCarros[10], spriteCarroQueimado, SpritesExplosao[50];
    Animation2  ListaExplosoes[MAX_EXPLOSOES];
    int         TimerGeral;
    int         carrosColididos;
    Carro       carros[QTD_CARROS];
    int         DistanciaRecorde, Geracao;
    int         valorDesfazerPista, maiorDistancia;
    double      velocidadeLaser;
    int         Fonte;
    double      Periodo = 0.01;
    int         spriteEstrelasLigada, spriteEstrelasDesligada, estrelasOpacidade = 255;
    int         spriteEspinho, spriteLama, spriteTurbo;

    PIG_Cor cores[6] = {AMARELO,CIANO,VERMELHO,AZUL,VERDE,ROXO};

    Obstaculo   obstaculos[QTD_OBSTACULO];
    Zona        zonas[QTD_ZONA];

    int         SpriteNeuronDesativado, SpriteNeuronAtivado, SpriteLuzAmarelo, SpriteLuzAzul, SpriteSeta;
    int         spriteContornoNeuronio;

int buscarMelhorCarroVivo()
{
    int menor = 999999;
    int indice = 0;
    for(int i=0; i<QTD_CARROS; i++)
    {
        if(carros[i].Colidiu == 0)
        {
            int dist = distancias[(int)carros[i].X][(int)carros[i].Y];
            if(dist < menor)
            {
                menor = dist;
                indice = i;
            }
        }
    }
    return indice;
}

int buscarMelhorCarro()
{
    int menor = 999999;
    int indice = 0;
    for(int i=0; i<QTD_CARROS; i++)
    {
        int dist = distancias[(int)carros[i].X][(int)carros[i].Y];
        if(dist < menor)
        {
            menor = dist;
            indice = i;
        }
    }
    return indice;
}

int buscarMelhorFitness()
{
    int maior = carros[0].Fitness;
    int indice = 0;
    for(int i=1; i<QTD_CARROS; i++)
    {
        double fit = carros[i].Fitness;
        if(fit > maior)
        {
            maior = fit;
            indice = i;
        }
    }
    return indice;
}

void aplicarSensores(Carro* carro, double* entrada)
{
    int i;
    for(i=0; i<CAR_BRAIN_QTD_INPUT-1; i++)
    {
        double X1 = carro->X;
        double Y1 = carro->Y;
        double Angulo = carro->Angulo - 90.0 + ((double)i)*180.0/((double)(CAR_BRAIN_QTD_INPUT-2));

        double Adjacente = 1*(cos(Angulo*M_PI/180.0));
        double Oposto = 1*(sin(Angulo*M_PI/180.0));

        while(1)
        {
            X1 = X1 + Adjacente;
            Y1 = Y1 + Oposto;

            if(cenario[(int)X1][(int)Y1] == 0)
            {
                X1 = X1 - Adjacente;
                Y1 = Y1 - Oposto;

                double dist = DistanciaEntrePontos(carro->X, carro->Y, X1, Y1);
                if(entrada != NULL)
                {
                    entrada[i] = dist;
                }
                carro->DistanciaSensores[i] = dist;
                break;
            }
        }
    }
    if(entrada != NULL)
    {
        entrada[i] = carro->Velocidade;
    }
}

PIG_Cor calcularCor(double Intensidade, PIG_Cor CorBase)
{
    CorBase.r = CorBase.r*Intensidade;
    CorBase.g = CorBase.g*Intensidade;
    CorBase.b = CorBase.b*Intensidade;

    return CorBase;
}

void InicializarSpritesNeuronios()
{
    SpriteNeuronDesativado = CriarSprite("imagens//neuronio7.png",0);
    DefinirColoracao(SpriteNeuronDesativado, PRETO);

    SpriteNeuronAtivado = CriarSprite("imagens//neuronio7.png",0);
    DefinirColoracao(SpriteNeuronAtivado, BRANCO);

    spriteContornoNeuronio = CriarSprite("imagens//branco.png",0);

    SpriteLuzAmarelo = CriarSprite("imagens//luz.png",0);

    SpriteLuzAzul = CriarSprite("imagens//luzAzul.png",0);

    SpriteSeta = CriarSprite("imagens\\seta2.png",0);
    DefinirColoracao(SpriteSeta, PRETO);
}

void DesenharRedeNeural(int X, int Y, int Largura, int Altura)
{
    double NeuroEntradaX[CAR_BRAIN_QTD_INPUT];
    double NeuroEntradaY[CAR_BRAIN_QTD_INPUT];
    double NeuroEscondidoX[CAR_BRAIN_QTD_LAYERS][CAR_BRAIN_QTD_HIDE];
    double NeuroEscondidoY[CAR_BRAIN_QTD_LAYERS][CAR_BRAIN_QTD_HIDE];
    double NeuroSaidaX[CAR_BRAIN_QTD_OUTPUT];
    double NeuroSaidaY[CAR_BRAIN_QTD_OUTPUT];

    double Entrada[CAR_BRAIN_QTD_INPUT];
    double XOrigem = X + 325;
    double YOrigem = Y + Altura;
    double LarguraPintura = Largura;
    double AlturaPintura = Altura;
    double TamanhoNeuronio = 20;
    char String[1000];
    int Sprite;

    int indice = buscarMelhorCarroVivo();
    Carro* melhorCarro = &carros[indice];

    int qtdEscondidas       = melhorCarro->Cerebro->QuantidadeEscondidas;
    int qtdNeuroEntrada     = melhorCarro->Cerebro->CamadaEntrada.QuantidadeNeuronios;
    int qtdNeuroEscondidas  = melhorCarro->Cerebro->CamadaEscondida[0].QuantidadeNeuronios;
    int qtdNeuroSaida       = melhorCarro->Cerebro->CamadaSaida.QuantidadeNeuronios;

    for(int i=0; i<CAR_BRAIN_QTD_INPUT; i++)
    {
        Entrada[i] = melhorCarro->Cerebro->CamadaEntrada.Neuronios[i].Saida;
    }

    double EscalaAltura = ((double)AlturaPintura)/(double)(qtdNeuroEscondidas-1);
    double EscalaLargura = ((double)LarguraPintura-475)/(double)(qtdEscondidas+1);

    double temp = YOrigem - (EscalaAltura*(qtdNeuroEscondidas-2))/2.0 + (EscalaAltura*(qtdNeuroSaida-1))/2.0;

    sprintf(String,"Acelerar");
    EscreverEsquerda(String, X + Largura - 130, temp - 0*EscalaAltura -5 - DESLOCAMENTO_NEURONIOS, Fonte);

    sprintf(String,"Ré");
    EscreverEsquerda(String, X + Largura - 130, temp - 1*EscalaAltura -5 - DESLOCAMENTO_NEURONIOS, Fonte);

    sprintf(String,"Virar Esquerda");
    EscreverEsquerda(String, X + Largura - 130, temp - 2*EscalaAltura -5 - DESLOCAMENTO_NEURONIOS, Fonte);

    sprintf(String,"Virar Direita");
    EscreverEsquerda(String, X + Largura - 130, temp - 3*EscalaAltura -5 - DESLOCAMENTO_NEURONIOS, Fonte);

    /// Desenhar Conexoes

    for(int i=0; i<qtdNeuroEntrada-1; i++)
    {
        NeuroEntradaX[i] = XOrigem;
        NeuroEntradaY[i] = YOrigem - i*EscalaAltura;
    }

    for(int i=0; i<qtdEscondidas; i++)
    {
        int qtdCamadaAnterior;
        Camada* CamadaAnterior;
        double *XAnterior, *YAnterior;

        if(i == 0)
        {
            qtdCamadaAnterior = qtdNeuroEntrada;
            CamadaAnterior = &melhorCarro->Cerebro->CamadaEntrada;
            XAnterior = NeuroEntradaX;
            YAnterior = NeuroEntradaY;
        }
        else
        {
            qtdCamadaAnterior = qtdNeuroEscondidas;
            CamadaAnterior = &melhorCarro->Cerebro->CamadaEscondida[i-1];
            XAnterior = NeuroEscondidoX[i-1];
            YAnterior = NeuroEscondidoY[i-1];
        }

        for(int j=0; j<qtdNeuroEscondidas-1; j++)
        {
            NeuroEscondidoX[i][j] = XOrigem + (i+1)*EscalaLargura;
            NeuroEscondidoY[i][j] = YOrigem - j*EscalaAltura - DESLOCAMENTO_NEURONIOS;

//            Sprite = SpriteNeuronDesativado;
//            double SaidaNeuronio = MelhorDinossauro->Cerebro->CamadaEscondida[i].Neuronios[j].Saida;
//            if(SaidaNeuronio > 0)
//            {
//                Sprite = SpriteNeuronAtivado;
//            }

            for(int k=0; k<qtdCamadaAnterior-1; k++)
            {
                double Peso = melhorCarro->Cerebro->CamadaEscondida[i].Neuronios[j].Peso[k];
                double Saida = CamadaAnterior->Neuronios[k].Saida;
                if(Peso*Saida > 0)
                {
                    DesenharLinhaSimples(XAnterior[k],
                                         YAnterior[k],
                                         NeuroEscondidoX[i][j],
                                         NeuroEscondidoY[i][j], VERMELHO);

                }
                else
                {
                    DesenharLinhaSimples(XAnterior[k],
                                         YAnterior[k],
                                         NeuroEscondidoX[i][j],
                                         NeuroEscondidoY[i][j], CINZA);
                }
            }

//            DesenharSprite(Sprite,
//                           NeuroEscondidoX[i][j],
//                           NeuroEscondidoY[i][j],
//                           TamanhoNeuronio,
//                           TamanhoNeuronio, 0, 0);
        }

    }

    for(int i=0; i<qtdNeuroSaida; i++)
    {
        int UltimaCamada = melhorCarro->Cerebro->QuantidadeEscondidas-1;
        double temp = YOrigem - (EscalaAltura*(qtdNeuroEscondidas-2))/2.0 + (EscalaAltura*(qtdNeuroSaida-1))/2.0;

        NeuroSaidaX[i] = XOrigem + (qtdEscondidas+1)*EscalaLargura;
        NeuroSaidaY[i] = temp - i*EscalaAltura - DESLOCAMENTO_NEURONIOS;

//        Sprite = SpriteNeuronDesativado;
//        double SaidaNeuronio = MelhorDinossauro->Cerebro->CamadaSaida.Neuronios[i].Saida;
//        if(SaidaNeuronio > 0.5)
//        {
//            Sprite = SpriteNeuronAtivado;
//        }

        for(int k=0; k<qtdNeuroEscondidas-1; k++)
        {
            double Peso = melhorCarro->Cerebro->CamadaSaida.Neuronios[i].Peso[k];
            double Saida = melhorCarro->Cerebro->CamadaEscondida[UltimaCamada].Neuronios[k].Saida;

            if(Peso*Saida > 0)
            {
                DesenharLinhaSimples(NeuroEscondidoX[UltimaCamada][k],
                                     NeuroEscondidoY[UltimaCamada][k],
                                     NeuroSaidaX[i],
                                     NeuroSaidaY[i], VERMELHO);


            }
            else
            {
                DesenharLinhaSimples(NeuroEscondidoX[UltimaCamada][k],
                                     NeuroEscondidoY[UltimaCamada][k],
                                     NeuroSaidaX[i],
                                     NeuroSaidaY[i], CINZA);
            }
        }
//        DesenharSprite(Sprite,
//                       NeuroSaidaX[i],
//                       NeuroSaidaY[i],
//                       TamanhoNeuronio,
//                       TamanhoNeuronio, 0, 0);
    }

    /// Desenhar Neuronios

    for(int i=0; i<qtdNeuroEntrada-1; i++)
    {
//        NeuroEntradaX[i] = XOrigem;
//        NeuroEntradaY[i] = YOrigem - i*EscalaAltura;

        PIG_Cor cor;
        double Opacidade;

        if(i == CAR_BRAIN_QTD_INPUT-1)
        {
            if(Entrada[i] > 15)
            {
                Opacidade = 1;
            }
            else
            {
                Opacidade = fabs(Entrada[i])/15.0;
            }
            cor = calcularCor(Opacidade, BRANCO);
        }
        else
        {
            if(Entrada[i] > 200.0)
            {
                Opacidade = 0;
            }
            else
            {
                Opacidade = fabs(200.0-Entrada[i])/200.0;
            }
            cor = calcularCor(Opacidade, BRANCO);
        }

        DefinirColoracao(SpriteNeuronAtivado, cor);
        DefinirOpacidade(SpriteLuzAmarelo, Opacidade*255);

//        DesenharSprite(SpriteLuzAmarelo,
//                       NeuroEntradaX[i],
//                       NeuroEntradaY[i],
//                       3.5*TamanhoNeuronio,
//                       3.5*TamanhoNeuronio, 0, 0);

        DesenharSprite(spriteContornoNeuronio,
                       NeuroEntradaX[i],
                       NeuroEntradaY[i],
                       TamanhoNeuronio*1.1,
                       TamanhoNeuronio*1.1, 0, 0);

        DesenharSprite(SpriteNeuronAtivado,
                       NeuroEntradaX[i],
                       NeuroEntradaY[i],
                       TamanhoNeuronio,
                       TamanhoNeuronio, 0, 0);

        //DefinirOpacidade(SpriteLuzAmarelo, 255);
        DefinirColoracao(SpriteNeuronAtivado, BRANCO);

//        DesenharSprite(SpriteSeta,
//                       NeuroEntradaX[i] - 56,
//                       NeuroEntradaY[i],
//                       64/(CameraZoom+1),
//                       12/(CameraZoom+1), 0);
    }

    for(int i=0; i<qtdEscondidas; i++)
    {
//        int qtdCamadaAnterior;
//        Camada* CamadaAnterior;
//        double *XAnterior, *YAnterior;
//
//        if(i == 0)
//        {
//            qtdCamadaAnterior = qtdNeuroEntrada;
//            CamadaAnterior = &MelhorDinossauro->Cerebro->CamadaEntrada;
//            XAnterior = NeuroEntradaX;
//            YAnterior = NeuroEntradaY;
//        }
//        else
//        {
//            qtdCamadaAnterior = qtdNeuroEscondidas;
//            CamadaAnterior = &MelhorDinossauro->Cerebro->CamadaEscondida[i-1];
//            XAnterior = NeuroEscondidoX[i-1];
//            YAnterior = NeuroEscondidoY[i-1];
//        }

        for(int j=0; j<qtdNeuroEscondidas-1; j++)
        {
//            NeuroEscondidoX[i][j] = XOrigem + (i+1)*EscalaLargura;
//            NeuroEscondidoY[i][j] = YOrigem - j*EscalaAltura;

            Sprite = SpriteNeuronDesativado;
            double SaidaNeuronio = melhorCarro->Cerebro->CamadaEscondida[i].Neuronios[j].Saida;
            if(SaidaNeuronio > 0)
            {
                Sprite = SpriteNeuronAtivado;
//                DesenharSprite( SpriteLuzAmarelo,
//                                NeuroEscondidoX[i][j],
//                                NeuroEscondidoY[i][j],
//                                3.5*TamanhoNeuronio,
//                                3.5*TamanhoNeuronio, 0, 0);
            }

//            for(int k=0; k<qtdCamadaAnterior-1; k++)
//            {
//                double Peso = MelhorDinossauro->Cerebro->CamadaEscondida[i].Neuronios[j].Peso[k];
//                double Saida = CamadaAnterior->Neuronios[k].Saida;
//                if(Peso*Saida > 0)
//                {
//                    DesenharLinhaSimples(XAnterior[k],
//                                         YAnterior[k],
//                                         NeuroEscondidoX[i][j],
//                                         NeuroEscondidoY[i][j], VERMELHO);
//
//                }
//                else
//                {
//                    DesenharLinhaSimples(XAnterior[k],
//                                         YAnterior[k],
//                                         NeuroEscondidoX[i][j],
//                                         NeuroEscondidoY[i][j], PRETO);
//                }
//            }


            DesenharSprite(spriteContornoNeuronio,
                           NeuroEscondidoX[i][j],
                           NeuroEscondidoY[i][j],
                           TamanhoNeuronio*1.1,
                           TamanhoNeuronio*1.1, 0, 0);

            DesenharSprite(Sprite,
                           NeuroEscondidoX[i][j],
                           NeuroEscondidoY[i][j],
                           TamanhoNeuronio,
                           TamanhoNeuronio, 0, 0);
        }
    }

    for(int i=0; i<qtdNeuroSaida; i++)
    {
//        int UltimaCamada = MelhorDinossauro->Cerebro->QuantidadeEscondidas-1;
//        double temp = YOrigem - (EscalaAltura*(qtdNeuroEscondidas-2))/2.0 + (EscalaAltura*(qtdNeuroSaida-1))/2.0;
//
//        NeuroSaidaX[i] = XOrigem + (qtdEscondidas+1)*EscalaLargura;
//        NeuroSaidaY[i] = temp - i*EscalaAltura;

        Sprite = SpriteNeuronDesativado;
        double SaidaNeuronio = melhorCarro->Cerebro->CamadaSaida.Neuronios[i].Saida;
        if(SaidaNeuronio > 0.5)
        {
            Sprite = SpriteNeuronAtivado;
//            DesenharSprite(SpriteLuzAmarelo,
//                       NeuroSaidaX[i],
//                       NeuroSaidaY[i],
//                       3.5*TamanhoNeuronio,
//                       3.5*TamanhoNeuronio, 0, 0);
        }

//        for(int k=0; k<qtdNeuroEscondidas-1; k++)
//        {
//            double Peso = MelhorDinossauro->Cerebro->CamadaSaida.Neuronios[i].Peso[k];
//            double Saida = MelhorDinossauro->Cerebro->CamadaEscondida[UltimaCamada].Neuronios[k].Saida;
//
//            if(Peso*Saida > 0)
//            {
//                DesenharLinhaSimples(NeuroEscondidoX[UltimaCamada][k],
//                                     NeuroEscondidoY[UltimaCamada][k],
//                                     NeuroSaidaX[i],
//                                     NeuroSaidaY[i], VERMELHO);
//            }
//            else
//            {
//                DesenharLinhaSimples(NeuroEscondidoX[UltimaCamada][k],
//                                     NeuroEscondidoY[UltimaCamada][k],
//                                     NeuroSaidaX[i],
//                                     NeuroSaidaY[i], PRETO);
//            }
//        }
        DesenharSprite(spriteContornoNeuronio,
                       NeuroSaidaX[i],
                       NeuroSaidaY[i],
                       TamanhoNeuronio*1.1,
                       TamanhoNeuronio*1.1, 0, 0);


        DesenharSprite(Sprite,
                       NeuroSaidaX[i],
                       NeuroSaidaY[i],
                       TamanhoNeuronio,
                       TamanhoNeuronio, 0, 0);
    }

}




void CriarTexturaExplosoes()
{
    char Caminho[200];
    int i;

    //PIG_Cor Cor = ((PIG_Cor){150,50,150,255});
    //int Opacidade = 150;

    for(i=0;i<44;i++)
    {
        sprintf(Caminho,"imagens\\explosao\\exp (%d).png",i+1);
        SpritesExplosao[i] = CriarSprite(Caminho,0);
        //DefinirColoracao(SpritesExplosao[i],Cor);
        //DefinirOpacidade(SpritesExplosao[i],Opacidade);
    }
}

void CriarExplosao(int X, int Y, int Largura, int Altura, double Angulo)
{
    int i;

    for(i=0; i<MAX_EXPLOSOES; i++)
    {
        if(ListaExplosoes[i].Ativada == 0)
        {
            ListaExplosoes[i].Ativada = 1;

            ListaExplosoes[i].X = X;
            ListaExplosoes[i].Y = Y;
            ListaExplosoes[i].Largura = Largura;
            ListaExplosoes[i].Altura = Altura;
            ListaExplosoes[i].FrameAtual = 0;
            ListaExplosoes[i].QuantidadeFrames = 44;
            ListaExplosoes[i].Angulo = Angulo;
            return;
        }
    }
}

void TrocarFrameExplosoes()
{
    int i;

    for(i=0; i<MAX_EXPLOSOES; i++)
    {
        if(ListaExplosoes[i].Ativada == 1)
        {
            ListaExplosoes[i].FrameAtual = ListaExplosoes[i].FrameAtual + 1;

            if(ListaExplosoes[i].FrameAtual > ListaExplosoes[i].QuantidadeFrames)
            {
                ListaExplosoes[i].Ativada = 0;
            }
        }
    }
}

void DesenharFrameExplosoes()
{
    int i;

    for(i=0; i<MAX_EXPLOSOES; i++)
    {
        if(ListaExplosoes[i].Ativada == 1)
        {
            double X = ListaExplosoes[i].X;
            double Y = ListaExplosoes[i].Y;

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            DesenharSprite(SpritesExplosao[ListaExplosoes[i].FrameAtual],
                           X,
                           Y,
                           ListaExplosoes[i].Largura,
                           ListaExplosoes[i].Altura,
                           ListaExplosoes[i].Angulo);
        }
    }
}











void desenhar()
{
    double X,Y;
    char String[1000];

    IniciarDesenho();

    DesenharSprite(spriteEstrelasDesligada,LARG_TELA/2,ALT_TELA/2,LARG_TELA,ALT_TELA,0, 0);
    DesenharSprite(spriteEstrelasLigada,LARG_TELA/2,ALT_TELA/2,LARG_TELA,ALT_TELA,0, 0);

    if(PIG_meuTeclado[TECLA_g])
    {
        int indice = buscarMelhorCarroVivo();
        CameraPosX = -carros[indice].X + LARG_TELA/2;
        CameraPosY = -carros[indice].Y + ALT_TELA/2;
    }

    /// Desenhar mapa de colisão ou spritePista
    if(PIG_meuTeclado[TECLA_z])
    {
        for(int i=0; i<quantidadeParede; i++)
        {
            X = paredes[i].x;
            Y = paredes[i].y;

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            paredesVirtual[i].x = X;
            paredesVirtual[i].y = ALT_TELA - Y;
        }

        DesenharPontos(paredesVirtual, quantidadeParede, BRANCO);
    }
    else
    {
        X = LARGURA_CENARIO/2;
        Y = ALTURA_CENARIO/2;
//
        X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
        Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

        DesenharSprite(spritePista, X, Y, LARGURA_CENARIO, ALTURA_CENARIO, 0);

        /// Desenhando obstaculos

        for(int i=0; i<QTD_OBSTACULO; i++)
        {
            X = obstaculos[i].X;
            Y = obstaculos[i].Y;

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            DesenharSprite(spriteEspinho,
                           X, Y,
                           obstaculos[i].tamanhoX, obstaculos[i].tamanhoY,
                           obstaculos[i].angulo, 1);
        }

        /// Desenhando zonas

        for(int i=0; i<QTD_ZONA; i++)
        {
            X = zonas[i].X;
            Y = zonas[i].Y;

            double X1 = X - zonas[i].larguraX/2;
            double Y1 = Y - zonas[i].larguraY/2;

            X1 = ((X1+CameraPosX)+((X1+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y1 = ((Y1+CameraPosY)+((Y1+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            int sprite;
            if(zonas[i].valor > 0)
            {
                sprite = spriteTurbo;
            }
            else
            {
                sprite = spriteLama;
            }

//            DesenharRetangulo(X1,
//                              Y1,
//                              zonas[i].larguraY*(CameraZoom+1),
//                              zonas[i].larguraX*(CameraZoom+1),VERDE);

            DesenharSprite(sprite,
                           X, Y,
                           zonas[i].larguraX, zonas[i].larguraY,
                           zonas[i].angulo, 1);
        }
    }

    /// Desenhar carros colididos

    for(int i=0; i<QTD_CARROS; i++)
    {
        if(carros[i].Colidiu == 1)
        {
            X = carros[i].X;
            Y = carros[i].Y;

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            int Altura = 20;
            if(carros[i].Queimado == 1)
            {
                DesenharSprite(spriteCarroQueimado, X, Y, Altura*2.1, Altura, carros[i].Angulo);
            }
            else
            {
                DesenharSprite(carros[i].Sprite, X, Y, Altura*2.1, Altura, carros[i].Angulo);
            }
        }
    }
//
    /// Desenhar carros vivos

    for(int i=0; i<QTD_CARROS; i++)
    {
        if(carros[i].Colidiu == 0)
        {
            X = carros[i].X;
            Y = carros[i].Y;

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            int Altura = 20;
            if(carros[i].Queimado == 1)
            {
                DesenharSprite(spriteCarroQueimado, X, Y, Altura*2.1, Altura, carros[i].Angulo);
            }
            else
            {
                DesenharSprite(carros[i].Sprite, X, Y, Altura*2.1, Altura, carros[i].Angulo);
            }
        }
    }

    /// Desenhar sensores do melhorCarro
    if(PIG_meuTeclado[TECLA_c])
    {
        int indice = buscarMelhorCarroVivo();

        aplicarSensores(&carros[indice], NULL);

        double X1 = carros[indice].X;
        double Y1 = carros[indice].Y;

        X1 = ((X1+CameraPosX)+((X1+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
        Y1 = ((Y1+CameraPosY)+((Y1+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

        for(int i=0; i<CAR_BRAIN_QTD_INPUT-1; i++)
        {
            X = carros[indice].X + carros[indice].DistanciaSensores[i]*cos(DEGTORAD*(carros[indice].Angulo - 90.0 + ((180.0/(CAR_BRAIN_QTD_INPUT-2))*i)));
            Y = carros[indice].Y + carros[indice].DistanciaSensores[i]*sin(DEGTORAD*(carros[indice].Angulo - 90.0 + ((180.0/(CAR_BRAIN_QTD_INPUT-2))*i)));

            int XNumero = X;
            int YNumero = Y - 10;

            XNumero = ((XNumero+CameraPosX)+((XNumero+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            YNumero = ((YNumero+CameraPosY)+((YNumero+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
            Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

            if(i < CAR_BRAIN_QTD_INPUT/4 || i > 3*CAR_BRAIN_QTD_INPUT/4)
            {
                DesenharLinhaSimples(X1,Y1,X,Y,cores[0]);
                DefinirOpacidade(SpriteLuzAmarelo,255);
                DesenharSprite(SpriteLuzAmarelo,X,Y,15,15,0,1);
            }
            else
            {
                DesenharLinhaSimples(X1,Y1,X,Y,cores[1]);
                DesenharSprite(SpriteLuzAzul,X,Y,15,15,0,1);
            }

            //sprintf(String,"%.0f", carros[indice].DistanciaSensores[i]);
            //EscreverCentralizada(String, XNumero, YNumero, Fonte);
        }
    }

    /// Desenhar matrizDistancias
    if(PIG_meuTeclado[TECLA_x])
    {
        int MouseX = XVirtualParaReal(PIG_evento.mouse.posX, PIG_evento.mouse.posY);
        int MouseY = YVirtualParaReal(PIG_evento.mouse.posX, PIG_evento.mouse.posY);

        for(int i=-25; i<25; i++)
        {
            for(int j=-25; j<25; j++)
            {
                X = i+MouseX;
                Y = j+MouseY;

                sprintf(String,"%d", distancias[(int)X][(int)Y]);

                X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
                Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

                EscreverCentralizada(String, X, Y, Fonte);
            }
        }
    }

    /// Desenhar Laser
    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            if(cenario[i][j] == 1)
            {
                if(distancias[i][j] == valorDesfazerPista)
                {
                    X = i;
                    Y = j;

                    X = ((X+CameraPosX)+((X+CameraPosX)-(LARG_TELA/2.0))*CameraZoom);
                    Y = ((Y+CameraPosY)+((Y+CameraPosY)-(ALT_TELA/2.0))*CameraZoom);

                    if(3*(CameraZoom+1) < 1)
                    {
                        DesenharRetangulo(X,Y,1,1,VERDE);
                    }
                    else
                    {
                        DesenharRetangulo(X,Y,3*(CameraZoom+1),3*(CameraZoom+1),VERDE);
                    }

                }
            }
        }
    }

    DesenharFrameExplosoes();
    DesenharRedeNeural(-230,660,600,75);

    EncerrarDesenho();
}

void movimentarCamera()
{
    double A = 10.0/(CameraZoom+1);
    if(PIG_meuTeclado[TECLA_w] == 1)
    {
        CameraPosY = CameraPosY - A;
    }
    if(PIG_meuTeclado[TECLA_s] == 1)
    {
        CameraPosY = CameraPosY + A;
    }
    if(PIG_meuTeclado[TECLA_a] == 1)
    {
        CameraPosX = CameraPosX + A;
    }
    if(PIG_meuTeclado[TECLA_d] == 1)
    {
        CameraPosX = CameraPosX - A;
    }

    double B = 0.2;
    double C = (B/(3.0/(CameraZoom+1)));
    if(PIG_meuTeclado[TECLA_q] == 1)
    {
        if(CameraZoom >= 0)
        {
            CameraZoom = CameraZoom + B;
        }
        else
        {
            CameraZoom = CameraZoom + C;
        }

        if(CameraZoom >= 50)
        {
            CameraZoom = 50;
        }
    }
    if(PIG_meuTeclado[TECLA_e] == 1)
    {
        if(CameraZoom >= 0)
        {
            CameraZoom = CameraZoom - B;
        }
        else
        {
            CameraZoom = CameraZoom - C;
        }

        if(CameraZoom <= -0.9999)
        {
            CameraZoom = -0.9999;
        }
    }
}

PIG_Cor getPixelColor(int objeto, int coluna, int linha)
{
    PIG_Cor cor;

    int largura = PegarLargura(objeto);
    int altura = PegarAltura(objeto);

    cor.a = 255;
    cor.r = CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].r;
    cor.g = CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].g;
    cor.b = CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].b;

    return cor;
}

void setPixelColor(int objeto, int coluna, int linha, PIG_Cor cor)
{
    int largura = PegarLargura(objeto);
    int altura = PegarAltura(objeto);

    CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].r = cor.r;
    CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].g = cor.g;
    CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].b = cor.b;
    CGerenciadorObjetos::objetos[objeto]->pixels[altura-1-linha][coluna].a = 255;
}

void inicializarSprites()
{
    spritePista = CriarSprite("imagens\\pistaObstaculo.png", 0);
    spriteCarros[0] = CriarSprite("imagens\\carro0.png",0);
    spriteCarros[1] = CriarSprite("imagens\\carro1.png",0);
    spriteCarros[2] = CriarSprite("imagens\\carro2.png",0);
    spriteCarros[3] = CriarSprite("imagens\\carro3.png",0);
    spriteCarros[4] = CriarSprite("imagens\\carro4.png",0);
    spriteCarros[5] = CriarSprite("imagens\\carro5.png",0);
    spriteCarros[6] = CriarSprite("imagens\\carro6.png",0);
    spriteCarroQueimado = CriarSprite("imagens\\carroQueimado.png",0);
    spriteEstrelasLigada = CriarSprite("imagens\\star.bmp",1);
    spriteEstrelasDesligada = CriarSprite("imagens\\star2.bmp",0);
    spriteEspinho = CriarSprite("imagens\\spike.png",0);
    spriteLama = CriarSprite("imagens\\lama.png",0);
    spriteTurbo = CriarSprite("imagens\\turbo.png",0);

    CriarTexturaExplosoes();

    InicializarSpritesNeuronios();
}

void preencherMatrizColisao()
{
    PIG_Cor cor;
    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            cor = getPixelColor(spritePista, i, j);
            if(cor.r == 110 && cor.g == 110 && cor.b == 110 && cor.a == 255)
            {
                cenario[i][j] = 1;
            }
            else
            {
                cenario[i][j] = 0;
            }
        }
    }
}

void criarPontoDesenhoColisao()
{
    quantidadeParede = 0;

    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            if(cenario[i][j] == 1)
            {
                paredes[quantidadeParede].x = i;
                paredes[quantidadeParede].y = j;
                quantidadeParede++;
            }
        }
    }
}

double getRandomValue()
{
    return (rand()%20001/10.0) - 1000.0;
    //return (rand()%201/10.0) - 10.0;
    //return (rand()%2001/1000.0) - 1.0;
    //return (rand()%2001/10000.0) - 0.1;

    //return rand()%3 - 1;
}

void inicializarObstaculo
(
    Obstaculo* obs,
    double X, double Y,
    int emMovimento, double VX, double VY, double rangeMaxMovimento,
    double angulo, double velocidadeRotacao,
    double tamanhoX, double tamanhoY
)
{
    obs->angulo = angulo;
    obs->emMovimento = emMovimento;
    obs->rangeMaxMovimento = rangeMaxMovimento;
    obs->rangeAtualMovimento = 0;
    obs->tamanhoX = tamanhoX;
    obs->tamanhoY = tamanhoY;
    obs->velocidadeRotacao = velocidadeRotacao;
    obs->VX = VX;
    obs->VY = VY;
    obs->X = X;
    obs->Y = Y;

    int k = X - tamanhoX/2;
    int l = Y - tamanhoY/2;
    for(int i=k; i<k+tamanhoX; i++)
    {
        for(int j=l; j<l+tamanhoY; j++)
        {
            if(DistanciaEntrePontos(X,Y,i,j) < tamanhoX/2)
            {
                cenario[i][j] = 0;
            }
        }
    }
}

void inicializarObstaculos()
{
    int tamanho = 75;
    double velocidadeGiro = 5;

    inicializarObstaculo(&obstaculos[0], 310, 1868, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[1], 705, 1868, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[2], 500, 1930, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[3], 900, 1930, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);

    inicializarObstaculo(&obstaculos[4], 215, 467, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[5], 240, 410, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[6], 280, 369, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[7], 337, 350, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);

    inicializarObstaculo(&obstaculos[8], 335, 244, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[9], 392, 245, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[10], 434, 200, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[11], 427, 150, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);

    inicializarObstaculo(&obstaculos[12], 1385, 360, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[13], 1315, 470, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[14], 1385, 580, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[15], 1315, 690, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[16], 1385, 800, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[17], 1315, 910, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[18], 1385, 1020, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[19], 1315, 1130, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);

    inicializarObstaculo(&obstaculos[20], 1865, 825, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[21], 1560, 530, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
    inicializarObstaculo(&obstaculos[22], 1930, 235, 0, 0, 0, 0, 0, velocidadeGiro, tamanho, tamanho);
}



void inicializarCarro(int i, double X, double Y)
{
    carros[i].Angulo = 90;
    carros[i].X = X;
    carros[i].Y = Y;
    carros[i].VX = 0;
    carros[i].VY = 0;
    carros[i].Velocidade = 0;
    carros[i].Colidiu = 0;
    carros[i].Queimado = 0;

    RNA_CopiarVetorParaCamadas(carros[i].Cerebro, carros[i].DNA);
}

void alocarCarro(int i)
{
    int Tamanho;

    carros[i].Sprite = spriteCarros[rand()%QTD_SPRITES_CARROS];

    carros[i].Cerebro = RNA_CriarRedeNeural(   CAR_BRAIN_QTD_LAYERS,
                                               CAR_BRAIN_QTD_INPUT,
                                               CAR_BRAIN_QTD_HIDE,
                                               CAR_BRAIN_QTD_OUTPUT);

    Tamanho = RNA_QuantidadePesos(carros[i].Cerebro);

    carros[i].TamanhoDNA = Tamanho;
    carros[i].DNA = (double*)malloc(Tamanho*sizeof(double));

    for(int j=0; j<carros[i].TamanhoDNA; j++)
    {
        carros[i].DNA[j] = getRandomValue();
    }
}

void alocarCarros()
{
    for(int i=0; i<QTD_CARROS; i++)
    {
        alocarCarro(i);
    }
}

void inicializarNovaPartida()
{
    carrosColididos = 0;
    valorDesfazerPista = maiorDistancia;
    velocidadeLaser = 1;

    for(int i=0; i<QTD_CARROS; i++)
    {
        inicializarCarro(i, 1852 + rand()%95, 1200);

        /// 1225 até 1320
    }
}

int valorMenorVizinho(int i, int j)
{
    int vizinhos[8];

    vizinhos[0] = distancias[i-1][j+1];
    vizinhos[1] = distancias[i][j+1];
    vizinhos[2] = distancias[i+1][j+1];
    vizinhos[3] = distancias[i-1][j];

    vizinhos[4] = distancias[i+1][j];
    vizinhos[5] = distancias[i-1][j-1];
    vizinhos[6] = distancias[i][j-1];
    vizinhos[7] = distancias[i+1][j-1];

    int Menor = 9999999;
    int indice = -1;
    for(int i=0; i<8; i++)
    {
        if(vizinhos[i] < Menor && vizinhos[i] >= 0)
        {
            Menor = vizinhos[i];
            indice = i;
        }
    }
    return Menor;
}

void salvarDistanciasArquivo()
{
    FILE* f = fopen("matrizDistancias","wb");
    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            fwrite(&distancias[i][j],1,4,f);
        }
    }
    fclose(f);
}

void carregarDistanciasArquivo(FILE* f)
{
    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            fread(&distancias[i][j],1,4,f);
        }
    }
}

int buscarMaiorDistancia()
{
    int maior = 0;
    int indicek = 0, indicem = 0;
    for(int k=0; k<LARGURA_CENARIO; k++)
    {
        for(int m=0; m<ALTURA_CENARIO; m++)
        {
            if(cenario[k][m] == 1)
            {
                if(distancias[k][m] > maior && distancias[k][m] != 9999999)
                {
                    maior = distancias[k][m];
                    indicek = k;
                    indicem = m;
                }
            }
        }
    }
    //printf("(%d,%d)\n",indicek,indicem);
    return maior;
}

void zerarLinhaChegada()
{
    distancias[1346][266] = 0;
}

void preencherMatrizDistancias()
{
    int novo;

    FILE* f = fopen("matrizDistancias","rb");
    if(f == NULL)
    {
        for(int i=0; i<LARGURA_CENARIO; i++)
        {
            for(int j=0; j<ALTURA_CENARIO; j++)
            {
                distancias[i][j] = 9999999;
            }
        }

        zerarLinhaChegada();

        while(1)
        {
            int mudancas = 0;

            for(int k=0; k<LARGURA_CENARIO; k++)
            {
                for(int m=0; m<ALTURA_CENARIO; m++)
                {
                    if(cenario[k][m] == 1)
                    {
                        novo = 1 + valorMenorVizinho(k,m);
                        if(novo < distancias[k][m])
                        {
                            distancias[k][m] = novo;
                            mudancas++;
                        }
                    }
                }
            }

            //printf("Mudancas: %d\n",mudancas);

            if(mudancas == 0)
            {
                break;
            }
        }

        for(int i=0; i<LARGURA_CENARIO; i++)
        {
            for(int j=0; j<ALTURA_CENARIO; j++)
            {
                if(cenario[i][j] == 0)
                {
                    distancias[i][j] = -1;
                }
            }
        }

        zerarLinhaChegada();

        salvarDistanciasArquivo();
    }
    else
    {
        carregarDistanciasArquivo(f);
        fclose(f);
    }

    maiorDistancia = buscarMaiorDistancia();
}

void inicializarZona(Zona* zona, double X, double Y, double larguraX, double larguraY, double valor, double angulo)
{
    zona->larguraX = larguraX;
    zona->larguraY = larguraY;
    zona->X = X;
    zona->Y = Y;
    zona->valor = valor;
    zona->angulo = angulo;
}


void inicializarZonas()
{
    double valorIncremento = 0.75;
    double valorDecremento = -1;

    inicializarZona(&zonas[0], 1520, 1610, 75, 75, valorDecremento, 0);

    inicializarZona(&zonas[1], 130, 1230, 75, 75, valorIncremento, 270);
    inicializarZona(&zonas[2], 790, 350, 75, 75, valorDecremento, 0);
    inicializarZona(&zonas[3], 790, 900, 75, 75, valorDecremento, 0);
    inicializarZona(&zonas[4], 669, 630, 75, 75, valorIncremento, 90);

    inicializarZona(&zonas[5], 1042, 965, 75, 75, valorIncremento, 270);
    inicializarZona(&zonas[6], 1164, 600, 75, 75, valorDecremento, 90);
    inicializarZona(&zonas[7], 1040, 265, 75, 75, valorDecremento, 90);

    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            matrizBoost[i][j] = 0;

            for(int k=0; k<QTD_ZONA; k++)
            {
                double inicioX =    zonas[k].X - zonas[k].larguraX/2.0;
                double finalX =     zonas[k].X + zonas[k].larguraX/2.0;
                double inicioY =    zonas[k].Y - zonas[k].larguraY/2.0;
                double finalY =     zonas[k].Y + zonas[k].larguraY/2.0;

                if(inicioX <= i && i <= finalX && inicioY <= j && j <= finalY)
                {
                    matrizBoost[i][j] = zonas[k].valor;
                }
            }
        }
    }
}

void configuracoesIniciais()
{
    CriarJanela("Deep Cars",0);

    inicializarSprites();
    TimerGeral = CriarTimer();

    InicializarGeradorAleatorio();
    preencherMatrizColisao();
    printf("Matriz de colisao preenchida\n");
    preencherMatrizDistancias();
    printf("Matriz de distancias preenchida\n");

    inicializarObstaculos();
    inicializarZonas();

    criarPontoDesenhoColisao();

    alocarCarros();

    Fonte = CriarFonteNormal("..\\fontes\\arial.ttf", 15, VERDE, 0, PRETO);

    DistanciaRecorde    = 0;
    Geracao             = 0;

    inicializarNovaPartida();
}

void movimentarCarros()
{
    for(int i=0; i<QTD_CARROS; i++)
    {
        if(carros[i].Colidiu == 0)
        {
            double TempVelocidade;

            if(matrizBoost[(int)carros[i].X][(int)carros[i].Y] != 0)
            {
                if(matrizBoost[(int)carros[i].X][(int)carros[i].Y] < 0)
                {
                    TempVelocidade = carros[i].Velocidade/5.0;
                }
                else
                {
                    carros[i].Velocidade = carros[i].Velocidade + matrizBoost[(int)carros[i].X][(int)carros[i].Y];
                    TempVelocidade = carros[i].Velocidade;
                }
            }
            else
            {
                TempVelocidade = carros[i].Velocidade;
            }

            carros[i].VX = TempVelocidade*cos(DEGTORAD*carros[i].Angulo);
            carros[i].VY = TempVelocidade*sin(DEGTORAD*carros[i].Angulo);

            if(cenario[(int)(carros[i].X + carros[i].VX)]
                      [(int)(carros[i].Y + carros[i].VY)] == 0 ||

               cenario[(int)(carros[i].X + 18*cos(DEGTORAD*carros[i].Angulo) + carros[i].VX)]
                      [(int)(carros[i].Y + 18*sin(DEGTORAD*carros[i].Angulo) + carros[i].VY)] == 0 ||

               cenario[(int)(carros[i].X - 18*cos(DEGTORAD*carros[i].Angulo) + carros[i].VX)]
                      [(int)(carros[i].Y - 18*sin(DEGTORAD*carros[i].Angulo) + carros[i].VY)] == 0)
            {
                carros[i].Colidiu = 1;
                carrosColididos++;
            }
            else
            {
                carros[i].X = carros[i].X + carros[i].VX;
                carros[i].Y = carros[i].Y + carros[i].VY;
            }

            if(carros[i].Velocidade > 0)
            {
                carros[i].Velocidade = carros[i].Velocidade - 0.1;
                if(carros[i].Velocidade < 0)
                {
                    carros[i].Velocidade = 0;
                }
            }
        }
    }
}

void verificarInteracaoJogador()
{
    if(PIG_meuTeclado[TECLA_CIMA])
    {
        carros[0].Velocidade = carros[0].Velocidade + 0.2;
    }
    if(PIG_meuTeclado[TECLA_BAIXO])
    {
        carros[0].Velocidade = carros[0].Velocidade - 0.2;
        if(carros[0].Velocidade < -8)
        {
            carros[0].Velocidade = -8;
        }
    }
    if(PIG_meuTeclado[TECLA_ESQUERDA])
    {
        carros[0].Angulo = carros[0].Angulo + 5.0;
    }
    if(PIG_meuTeclado[TECLA_DIREITA])
    {
        carros[0].Angulo = carros[0].Angulo - 5.0;
    }





    if(PIG_meuTeclado[TECLA_KP_8])
    {
        carros[1].Velocidade = carros[1].Velocidade + 0.2;
    }
    if(PIG_meuTeclado[TECLA_KP_5])
    {
        carros[1].Velocidade = carros[1].Velocidade - 0.2;
    }
    if(PIG_meuTeclado[TECLA_KP_4])
    {
        carros[1].Angulo = carros[1].Angulo + 5.0;
    }
    if(PIG_meuTeclado[TECLA_KP_6])
    {
        carros[1].Angulo = carros[1].Angulo - 5.0;
    }



    if(PIG_meuTeclado[TECLA_t])
    {
        carros[2].Velocidade = carros[2].Velocidade + 0.2;
    }
    if(PIG_meuTeclado[TECLA_g])
    {
        carros[2].Velocidade = carros[2].Velocidade - 0.2;
    }
    if(PIG_meuTeclado[TECLA_f])
    {
        carros[2].Angulo = carros[2].Angulo + 5.0;
    }
    if(PIG_meuTeclado[TECLA_h])
    {
        carros[2].Angulo = carros[2].Angulo - 5.0;
    }


}

void carregarRedeArquivo()
{
    FILE* f = fopen("rede","rb");

    if(f == NULL)
    {
        return;
    }

    fread(&carros[0].TamanhoDNA, 1, sizeof(int), f);
    fread(carros[0].DNA, carros[0].TamanhoDNA, sizeof(double), f);
    RNA_CopiarVetorParaCamadas(carros[0].Cerebro, carros[0].DNA);

    fclose(f);

    //RNA_CopiarVetorParaCamadas(Dinossauros[0].Cerebro, Dinossauros[0].DNA);
}

void salvarRedeArquivo()
{
    char String[1000];
    int indice = buscarMelhorFitness();

    double fit = carros[indice].Fitness;
    double dist = (double)distancias[(int)carros[indice].X][(int)carros[indice].Y];

    printf("Salvando em arquivo, melhor da geracao: %f pixels\n",maiorDistancia-dist);

    sprintf(String, "redes\\%.2f - [%d,%d,%d,%d]",
            fit,
            CAR_BRAIN_QTD_LAYERS,
            CAR_BRAIN_QTD_INPUT,
            CAR_BRAIN_QTD_HIDE,
            CAR_BRAIN_QTD_OUTPUT);

    FILE* f = fopen(String,"wb");

    fwrite(&carros[indice].TamanhoDNA,  1,                              sizeof(int), f);
    fwrite(carros[indice].DNA,         carros[indice].TamanhoDNA,       sizeof(double), f);

    fclose(f);
}


void aplicarSaida(Carro* carro, double* saida)
{
    if(saida[0] > 0)
    {
        carro->Velocidade = carro->Velocidade + 0.2;
    }
    if(saida[1] > 0)
    {
        carro->Velocidade = carro->Velocidade - 0.2;
        if(carro->Velocidade < -4)
        {
            carro->Velocidade = -4;
        }
    }
    if(saida[2] > 0)
    {
        carro->Angulo = carro->Angulo + 5.0;
    }
    if(saida[3] > 0)
    {
        carro->Angulo = carro->Angulo - 5.0;
    }
}


void controlarCarros()
{
    double Saida[CAR_BRAIN_QTD_OUTPUT];
    double Entrada[CAR_BRAIN_QTD_INPUT];

    for(int i=0; i<QTD_CARROS; i++)
    {
        if(carros[i].Colidiu == 0)
        {
            aplicarSensores(&carros[i], Entrada);

            RNA_CopiarParaEntrada(carros[i].Cerebro, Entrada);
            RNA_CalcularSaida(carros[i].Cerebro);
            RNA_CopiarDaSaida(carros[i].Cerebro, Saida);

            if(carros[i].Colidiu == 0)
            {
                aplicarSaida(&carros[i], Saida);
            }
        }
    }
}

void encerrarPartida()
{
    double dist;
    double tempo;
    for(int i=0; i<QTD_CARROS; i++)
    {
        carros[i].Colidiu = 1;

        dist = (double)maiorDistancia - (double)distancias[(int)carros[i].X][(int)carros[i].Y];

        carros[i].Fitness = dist;
    }

    salvarRedeArquivo();
}

void randomMutations()
{
    static double RangeRandom = carros[0].TamanhoDNA;

    carro* Vetor[QTD_CARROS];
    carro* Temp;

    for(int i=0; i<QTD_CARROS; i++)
    {
        Vetor[i] = &carros[i];
    }

    for(int i=0; i<QTD_CARROS; i++)
    {
        for(int j=0; j<QTD_CARROS-1; j++)
        {
            if(Vetor[j]->Fitness < Vetor[j+1]->Fitness)
            {
                Temp = Vetor[j];
                Vetor[j] = Vetor[j+1];
                Vetor[j+1] = Temp;
            }
        }
    }

    printf("Melhor geracao: \t\t%f\n", Vetor[0]->Fitness);

    int Step = 5;
    for(int i=0; i<Step; i++)  /// Clonando individuos
    {
        for(int j=Step+i; j<QTD_CARROS; j=j+Step)
        {
            for(int k=0; k<Vetor[j]->TamanhoDNA; k++)
            {
                Vetor[j]->DNA[k] = Vetor[i]->DNA[k];        /// individuo 'j' recebe dna do individuo 'i'
            }
        }
    }

    for(int j=Step; j<QTD_CARROS; j++)
    {
        int tipo;
        int mutations = (rand()%(int)RangeRandom)+1;

        for(int k=0; k<mutations; k++)
        {
            tipo = rand()%3;

            int indice = rand()%Vetor[j]->TamanhoDNA;
            switch(tipo)
            {
                case 0:
                {
                    //int mutations = 20;
                    Vetor[j]->DNA[indice] = getRandomValue();       /// Valor Aleatorio

                }   break;
                case 1:
                {
                    double number = (rand()%10001)/10000.0 + 0.5;
                    Vetor[j]->DNA[indice] = Vetor[j]->DNA[indice]*number;   /// Multiplicação aleatoria

                }   break;
                case 2:
                {
                    double number = getRandomValue()/100.0;
                    Vetor[j]->DNA[indice] = Vetor[j]->DNA[indice] + number; /// Soma aleatoria

                }   break;
            }
        }
    }


    printf("Range Random: \t%.1f de %d\n", RangeRandom, carros[0].TamanhoDNA);
    RangeRandom = RangeRandom*0.999;
    if(RangeRandom < 15)
    {
        RangeRandom = 15;
    }

    //printf("5");
    Geracao++;
}

void verificarFimDePartida()
{
    if(carrosColididos == QTD_CARROS)
    {
        encerrarPartida();
        randomMutations();
        inicializarNovaPartida();
    }
}

void desfazerPista()
{
    for(int i=0; i<LARGURA_CENARIO; i++)
    {
        for(int j=0; j<ALTURA_CENARIO; j++)
        {
            if(distancias[i][j] <= valorDesfazerPista)
            {
                setPixelColor(spritePista, i,j, BRANCO);
            }
        }
    }
}

void laserDestruidor()
{
    for(int i=0; i<QTD_CARROS; i++)
    {
        if(distancias[(int)carros[i].X][(int)carros[i].Y] >= valorDesfazerPista)
        {
            if(carros[i].Colidiu == 0)
            {
                carros[i].Colidiu = 1;
                carrosColididos++;
            }
            if(carros[i].Queimado == 0)
            {
                CriarExplosao(carros[i].X, carros[i].Y, 250, 250, rand()%360);
                carros[i].Queimado = 1;
            }
        }
    }
    if(valorDesfazerPista > 0)
    {
        valorDesfazerPista = valorDesfazerPista - velocidadeLaser;
        velocidadeLaser = velocidadeLaser + 0.004;
    }
}

void atualizarOpacidadeBackground()
{
    static int sinal = 1;
    if(estrelasOpacidade >= 255 || estrelasOpacidade <= 0)
    {
        sinal = -sinal;
    }
    estrelasOpacidade = estrelasOpacidade + 5*sinal;
    DefinirOpacidade(spriteEstrelasLigada, estrelasOpacidade);
}

void verificarTeclasUsuario()
{
    if(PIG_Tecla == TECLA_n)
    {
        Periodo = Periodo/2;
        printf("Periodo: %f\n",Periodo);
    }
    if(PIG_Tecla == TECLA_m)
    {
        Periodo = Periodo*2;
        printf("Periodo: %f\n",Periodo);
    }
    if(PIG_Tecla == TECLA_F2)
    {
        carregarRedeArquivo();
    }
}

void girarObstaculos()
{
    for(int i=0; i<QTD_OBSTACULO; i++)
    {
        obstaculos[i].angulo = obstaculos[i].angulo + obstaculos[i].velocidadeRotacao;
    }
}

int main(int argc, char* args[])
{
    configuracoesIniciais();

    while(PIG_JogoRodando == 1)
    {
        AtualizarJanela();
        verificarTeclasUsuario();

        if(TempoDecorrido(TimerGeral) >= Periodo)
        {
            movimentarCamera();

            controlarCarros();
            movimentarCarros();

            girarObstaculos();

            verificarInteracaoJogador();

            laserDestruidor();

            TrocarFrameExplosoes();
            atualizarOpacidadeBackground();

            verificarFimDePartida();

            desenhar();

            ReiniciarTimer(TimerGeral);
        }
    }

    FinalizarJanela();

    return 0;
}

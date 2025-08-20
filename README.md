🎯 Sistema Automatizado de Carretilha com Queda Livre
Sistema integrado de controle automático para simulação de experimentos de queda livre utilizando ESP32, motor de passo NEMA 14, motor DC e carretilha de pesca.
📋 Índice
📖 Sobre o Projeto
Funcionamento
Aplicações
⚡ Características
🛠️ Componentes
🔧 Especificações Técnicas
🔌 Esquema de Ligação
⚙️ Instalação
💻 Código
🚀 Como Usar
🔧 Configuração
📊 Especificações
🐛 Solução de Problemas
🤝 Contribuição
📄 Licença
📞 Contato e Suporte
📖 Sobre o Projeto
Este projeto implementa um sistema automatizado completo que simula experimentos de queda livre de forma controlada e repetível. O sistema utiliza uma carretilha de pesca como mecanismo de liberação e recolhimento, controlada por um motor de passo para precisão e um motor DC para torque.
🎯 Funcionamento
Posição Inicial: Penduricalho suspenso por nylon, carretilha travada.
Destravamento: Motor de passo destrava a carretilha → início da queda livre.
Queda Livre: Objeto cai por tempo determinado (ex: 2 segundos).
Travamento: Motor de passo trava novamente a carretilha.
Recolhimento: Motor DC puxa a manivela, recolhendo o objeto.
Repetição: O ciclo se repete automaticamente.
🔬 Aplicações
Experimentos de física (queda livre, gravidade).
Demonstrações educacionais.
Testes de materiais em queda.
Automação industrial.
Projetos de robótica.
⚡ Características
✅ Sistema de Controle
Motor de Passo: Controle direto de posição via driver TB6600.
Motor DC: Acionamento liga/desliga via relé 12V.
Precisão: ±1.8° no travamento/destravamento da carretilha.
Automação: Ciclos de operação completamente automatizados.
✅ Recursos Avançados
🔄 Operação contínua e autônoma.
📊 Monitoramento em tempo real via porta Serial.
⚡ Alimentação unificada (12V para motores + 5V USB para ESP32).
🛡️ Sistema de segurança com diodo de proteção.
📈 Contadores de ciclo e estatísticas (implementação futura).
🔧 Parâmetros facilmente configuráveis no código.
✅ Controles Inteligentes
Máquina de Estados: Controle robusto da sequência de operações.
Temporizadores Precisos: Timing exato para cada etapa do ciclo.
Diagnósticos: Verificação automática de funcionamento no setup.
Recuperação: Sistema de reset e parada de emergência via reinicialização do ESP32.
🛠️ Componentes
📦 Lista de Materiais


Componente
Especificação
Qtd
Preço Aprox.
Função
ESP32 DevKit V1
30 pinos, WiFi/BT
1
R$ 25-35
Microcontrolador principal
Driver TB6600
4.5A, 9-42V
1
R$ 35-50
Controle do motor de passo
Motor NEMA 14
Bipolar, 1.8°/passo
1
R$ 40-60
Travamento da carretilha
Motor DC 12V
25 RPM, alto torque
1
R$ 30-45
Puxar manivela
Módulo Relé 12V
1 canal, 10A
1
R$ 8-15
Controle do motor DC
Fonte 12V 4A
Regulada, com proteção
1
R$ 25-40
Alimentação do sistema
Carretilha de Pesca
Com trava manual
1
R$ 50-80
Mecanismo de liberação
Diodo 1N4007
Proteção reversa
1
R$ 0,50
Proteção do motor DC
Jumpers/Cabos
Variados
-
R$ 10-20
Conexões

💰 Custo Total Estimado: R$ 223 - R$ 345
🔧 Especificações Técnicas
Motor de Passo NEMA 14
Tipo: Bipolar (4 fios)
Ângulo por Passo: 1.8°
Passos por Volta: 200 (em modo Full Step)
Corrente: 1.0-1.7A
Tensão: 12V
Torque: 14 N.cm
Controle: Direto via TB6600
Motor DC
Tensão: 12V DC
Rotação: 25 RPM
Torque: Alto, adequado para a carga de recolhimento
Corrente: ~1-2A
Controle: Liga/desliga via relé
Proteção: Diodo 1N4007 (flyback)
🔌 Esquema de Ligação
📐 Diagrama Completo
                      FONTE 12V (4A mínimo)
                     ┌─────────────────┐
                     │ +12V       GND  │
                     └──┬───────────┬──┘
                        │           │
   ┌────────────────────┼───────────┼────────────────────┐
   │                    │           │                    │
   ▼                    ▼           ▼                    ▼
┌─────────┐      ┌──────────┐   ┌─────────┐      ┌─────────────┐
│ ESP32   │◄────►│ TB6600   │   │ RELÉ    │◄────►│ MOTOR DC    │
│ DevKit  │      │ Driver   │   │ 12V     │      │ 12V/25RPM   │
└─────────┘      └─────┬────┘   └─────────┘      └─────────────┘
   │                   │
   │                   ▼
   │             ┌─────────┐
   │             │ NEMA 14 │
   │             │ Motor   │
   └───────────► └─────────┘


🔗 Tabela de Conexões
ESP32 ↔ TB6600 (Motor de Passo) | ESP32 Pin | TB6600 Pin | Função | | :--- | :--- | :--- | | GPIO 25 | PUL+ | Pulsos de passo | | GPIO 26 | DIR+ | Direção | | GPIO 27 | ENA+ | Habilitação | | GND | PUL-, DIR-, ENA- | Terra comum |
ESP32 ↔ Relé (Motor DC) | ESP32 Pin | Relé Pin | Função | | :--- | :--- | :--- | | GPIO 32 | IN | Controle do relé | | GND | GND | Terra |
Alimentação 12V | Fonte | Destino | Corrente (Aprox.) | Função | | :--- | :--- | :--- | :--- | | +12V | TB6600 VCC | ~2A | Alimentação do motor de passo | | +12V | Relé VCC | ~50mA | Alimentação da bobina do relé | | +12V | Relé COM | ~2A | Alimentação do motor DC | | GND | Todos os GND | - | Terra comum |
Proteções
Motor DC: Diodo 1N4007 em paralelo com os terminais do motor (cátodo/faixa no +12V).
Terra: É obrigatório conectar o GND da fonte 12V com o GND do ESP32.
Fusível: Um fusível de 5A é recomendado na entrada principal de +12V.
⚙️ Instalação
📋 Pré-requisitos
Arduino IDE 1.8.x ou superior.
Placa ESP32 instalada no Board Manager da IDE.
Driver USB (CH340/CP2102) correspondente ao seu ESP32.
Cabo de dados Micro-USB.
🚀 Configuração do Arduino IDE
Instalar Suporte ao ESP32:
Abra a Arduino IDE e vá em Arquivo → Preferências.
No campo "URLs Adicionais para Gerenciadores de Placas", adicione: https://dl.espressif.com/dl/package_esp32_index.json
Vá em Ferramentas → Placa → Gerenciador de Placas.
Procure por "ESP32" e instale o pacote "esp32 by Espressif Systems".
Selecione a placa em Ferramentas → Placa → ESP32 Dev Module.
Configurações da Placa:
Placa: "ESP32 Dev Module"
Upload Speed: "115200"
CPU Frequency: "240MHz (WiFi/BT)"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Port: Selecione a porta COM correspondente ao seu ESP32.
🔧 Configuração do Hardware
DIP Switches do TB6600:
┌─────────────────────────────────┐
│ MICROSTEPPING (Full Step):      │
│ S4: OFF   S5: OFF   S6: OFF     │
│                                 │
│ CORRENTE (ex: 1.5A):            │
│ S1: ON    S2: OFF   S3: OFF     │
└─────────────────────────────────┘

Consulte o manual do seu motor para ajustar a corrente corretamente.
Montagem Mecânica:
Acople o eixo do motor de passo ao mecanismo de trava da carretilha.
Acople o eixo do motor DC à manivela da carretilha.
Instale o sistema em uma estrutura estável.
Conecte o nylon ao objeto (penduricalho) que será usado no experimento.
Verifique todas as conexões elétricas antes de ligar a fonte.
💻 Código
📁 Estrutura do Projeto
carretilha-queda-livre/
├── carretilha_sistema.ino     # Código principal
├── README.md                  # Documentação do projeto
├── esquemas/                  # Diagramas de ligação
├── fotos/                     # Imagens do projeto
└── LICENSE                    # Licença de uso


🔽 Download e Upload
Clonar o Repositório:
git clone https://github.com/seu-usuario/carretilha-queda-livre.git
cd carretilha-queda-livre


Abrir e Fazer Upload:
Abra o arquivo carretilha_sistema.ino na Arduino IDE.
Verifique e compile o código (clicando no ✔️ ou Ctrl+R).
Conecte o ESP32 ao computador via USB.
Faça o upload do código (clicando na ➡️ ou Ctrl+U).
Abra o Monitor Serial (Ctrl+Shift+M) com a velocidade de 115200 baud para ver o status.
⚙️ Parâmetros Configuráveis
Ajuste os principais parâmetros diretamente no início do código:
// Ângulos de movimento do motor de passo
const int ANGULO_DESTRAVAMENTO = 60;   // Graus para destravar (sentido anti-horário)
const int ANGULO_TRAVAMENTO = 60;      // Graus para travar (sentido horário)

// Tempos da sequência (em milissegundos)
const int TEMPO_QUEDA_LIVRE = 2000;    // 2 segundos de queda
const int TEMPO_ESPERA_INICIAL = 5000; // 5 segundos de espera após travar
const int TEMPO_MOTOR_DC = 10000;      // 10 segundos de recolhimento
const int TEMPO_ESPERA_FINAL = 3000;   // 3 segundos de espera entre ciclos

// Velocidade do motor de passo (delay entre pulsos em microssegundos)
const int VELOCIDADE_MOVIMENTO = 1200; // Menor valor = mais rápido


🚀 Como Usar
🔍 Primeira Execução
Verificação Inicial:
✅ Confira todas as conexões elétricas.
✅ Ligue a fonte de 12V.
✅ Conecte o ESP32 ao PC via USB.
✅ Abra o Monitor Serial (115200 baud).
✅ Verifique se os DIP switches do TB6600 estão configurados corretamente.
Teste dos Componentes:
✅ O motor de passo deve girar conforme os comandos do código.
✅ O relé deve emitir um "click" ao ser ativado/desativado.
✅ O motor DC deve ligar e desligar quando o relé é acionado.
✅ O mecanismo da carretilha deve travar e destravar completamente.
🔄 Operação Normal
A sequência automatizada tem um ciclo total de aproximadamente 24 segundos:
Etapa
Duração
Ação
Motor Passo
Motor DC
1️⃣ Preparação
2s
Verificação dos sistemas
🔒 Travado
⏹️ Desligado
2️⃣ Destravamento
~1s
Gira 60° (anti-horário)
🔓 Girando
⏹️ Desligado
3️⃣ Queda Livre
2s
Objeto cai livremente
⏸️ Parado
⏹️ Desligado
4️⃣ Travamento
~1s
Gira 60° (horário)
🔒 Girando
⏹️ Desligado
5️⃣ Espera
5s
Pausa para estabilização
🔒 Travado
⏹️ Desligado
6️⃣ Recolhimento
10s
Puxa a manivela
🔒 Travado
⚡ Ligado
7️⃣ Finalização
3s
Prepara para o novo ciclo
🔒 Travado
⏹️ Desligado

📊 Monitoramento
O sistema fornece feedback detalhado via Monitor Serial a cada etapa:
╔══════════════════════════════════════════════════════════╗
║                      CICLO NÚMERO 1                      ║
╚══════════════════════════════════════════════════════════╝

[ETAPA 1] 🏁 PREPARAÇÃO DO SISTEMA
   • Motor de passo: Mantendo carretilha travada 🔒
   • Motor DC: Desligado via relé ⏹️
   • Penduricalho: Posição inicial (erguido) ✅
   ⏰ 2 segundo(s) para iniciar...

[ETAPA 2] 🔓 DESTRAVAMENTO DA CARRETILHA
   • Motor de passo: Girando 60° no sentido anti-horário
   → Executando 33 passos (60°) - Direção: Anti-horário ↺
   Progresso: [████████████████████] 100%
   ✅ Carretilha DESTRAVADA com sucesso!

[...]


🔧 Configuração
⚙️ Ajustes Comuns
Alterar o Tempo de Queda:
const int TEMPO_QUEDA_LIVRE = 3000; // Altera para 3 segundos


Modificar o Ângulo de Travamento:
const int ANGULO_TRAVAMENTO = 90;   // Altera para 90 graus


Ajustar a Velocidade do Motor de Passo:
const int VELOCIDADE_MOVIMENTO = 800; // Mais rápido (menor valor)


🔧 Calibração
Teste de Precisão do Motor de Passo: Para verificar se o motor está girando corretamente, adicione o seguinte código no setup() para um teste único:
moverMotorPasso(360, true); // Deve girar exatamente 1 volta no sentido anti-horário


Verificação do Relé: Teste o acionamento do relé e do motor DC com este código:
digitalWrite(RELAY_PIN, HIGH); // Liga o motor DC
delay(2000);
digitalWrite(RELAY_PIN, LOW);  // Desliga o motor DC
delay(2000);


Ajuste Mecânico:
Verifique se o ângulo configurado (ex: 60°) é suficiente para travar e destravar o mecanismo completamente.
Ajuste o acoplamento entre os motores e a carretilha se houver folga ou deslizamento.
📊 Especificações
⚡ Consumo de Energia (Estimado)
Estado
Motor Passo
Motor DC
Relé
Total
Repouso
0.2A
0A
0.05A
~0.25A
Movimento Passo
1.5A
0A
0.05A
~1.55A
Recolhimento
0.2A
1.5A
0.05A
~1.75A
Pico Máximo
1.5A
1.5A
0.05A
~3.05A

📏 Performance
Parâmetro
Valor
Unidade
Precisão Angular
±1.8
graus
Tempo de Ciclo Completo
~24
segundos
Altura Máxima de Queda
~3
metros
Peso Máximo do Objeto
~500
gramas
Resolução Temporal
50
milissegundos

🔌 Requisitos Elétricos
Fonte Principal: 12V DC, 4A (mínimo recomendado).
Alimentação ESP32: 5V via USB (pode ser alimentado separadamente).
Consumo Médio: ~1.2A.
Consumo de Pico: ~3.1A.
Proteção: Fusível de 5A na entrada principal.
🐛 Solução de Problemas
❌ Problemas Comuns
🔴 Motor de passo não se move
Causas: DIP switches do TB6600 incorretas; conexões PUL/DIR/ENA soltas; fonte 12V desligada; motor desabilitado no código.
Soluções: Verifique a configuração dos DIP switches (Full Step: S4/S5/S6 = OFF); confira as conexões ESP32 ↔ TB6600; meça a tensão na fonte; verifique se o pino ENA está em LOW para habilitar.
🔴 Motor DC não liga
Causas: Relé defeituoso ou mal conectado; fonte insuficiente; GPIO 32 não está ativando o relé.
Soluções: Teste o relé manualmente com 12V; verifique se o relé faz "click" ao ser ativado; meça a tensão no pino IN do relé (deve ir para 3.3V).
🔴 Sistema reinicia durante a operação
Causas: Fonte de 12V com corrente insuficiente para os picos de consumo; ruído elétrico.
Soluções: Use uma fonte de 4A ou superior; adicione um capacitor de 1000µF na entrada de 12V; verifique se todos os GNDs estão conectados.
🔴 Movimentos do motor de passo são imprecisos
Causas: Configuração de micropassos incorreta; velocidade muito alta (perda de passos); carga mecânica excessiva.
Soluções: Garanta que o driver está em modo Full Step (S4/S5/S6 = OFF); aumente o valor de VELOCIDADE_MOVIMENTO (torna o motor mais lento e forte); verifique se não há obstruções mecânicas.
🔍 Diagnósticos
Para um teste rápido de hardware, adicione esta função ao seu código e chame-a no setup():
void diagnosticoHardware() {
  Serial.println("=== INICIANDO DIAGNÓSTICO DE HARDWARE ===");
  
  // Teste do motor de passo (1 volta completa)
  Serial.println("Testando motor de passo...");
  moverMotorPasso(360, true);
  delay(1000);
  moverMotorPasso(360, false);
  
  // Teste do relé e motor DC
  Serial.println("Testando relé e motor DC...");
  digitalWrite(RELAY_PIN, HIGH); delay(2000);
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.println("=== DIAGNÓSTICO CONCLUÍDO ===");
}



🌟 Ideias para Melhorias Futuras
Interface Web: Controle e monitoramento via WiFi.
Sensores: Adicionar um sensor de fim de curso para o recolhimento.
Display LCD/OLED: Para exibir o status localmente.
Datalogger: Salvar os dados de cada experimento em um cartão SD.
Medição de Tempo: Usar sensores para medir o tempo de queda com precisão.
Câmera: Integrar uma câmera para gravação automática dos experimentos.


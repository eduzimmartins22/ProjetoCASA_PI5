        // Estado do sistema
        let ledState = false;
        let waterState = false;
        let waterLevel = 50;
        let currentColor = "#ff0000";
        let currentPosition = 0;
        let ledPositions = new Array(10).fill("#000000");

        // Funções LED
        function toggleLED() {
            ledState = !ledState;
            const btn = document.getElementById('led-status');
            btn.textContent = ledState ? 'DESLIGAR LED' : 'LIGAR LED';
            
            // Enviar comando para C++
            sendCommand('LED_TOGGLE', { state: ledState });
            updateStatus(`LEDs ${ledState ? 'ligados' : 'desligados'}`);
        }

        function updateLEDColor() {
            currentColor = document.getElementById('led-color').value;
            sendCommand('LED_COLOR', { color: currentColor });
            updateStatus(`Cor selecionada: ${currentColor}`);
        }

        function selectLEDPosition() {
            currentPosition = parseInt(document.getElementById('led-position').value);
            updateStatus(`Posição selecionada: LED ${currentPosition + 1}`);
        }

        function applyColorToPosition() {
            ledPositions[currentPosition] = currentColor;
            sendCommand('LED_SET_POSITION', { 
                position: currentPosition, 
                color: currentColor 
            });
            updateStatus(`Cor ${currentColor} aplicada no LED ${currentPosition + 1}`);
        }

        // Funções Jato d'Água
        function toggleWater() {
            waterState = !waterState;
            const btn = document.getElementById('water-status');
            btn.textContent = waterState ? 'DESLIGAR JATO D\'ÁGUA' : 'LIGAR JATO D\'ÁGUA';
            
            sendCommand('WATER_TOGGLE', { state: waterState });
            updateStatus(`Jato d'água ${waterState ? 'ligado' : 'desligado'}`);
        }

        function increaseWater() {
            if (waterLevel < 100) {
                waterLevel = Math.min(100, waterLevel + 10);
                updateWaterLevel();
                sendCommand('WATER_INCREASE', { level: waterLevel });
                updateStatus(`Jato aumentado para ${waterLevel}%`);
            }
        }

        function decreaseWater() {
            if (waterLevel > 0) {
                waterLevel = Math.max(0, waterLevel - 10);
                updateWaterLevel();
                sendCommand('WATER_DECREASE', { level: waterLevel });
                updateStatus(`Jato diminuído para ${waterLevel}%`);
            }
        }

        function updateWaterLevel() {
            document.getElementById('water-level').style.width = waterLevel + '%';
            document.getElementById('water-percentage').textContent = waterLevel + '%';
        }

        function updateStatus(message) {
            document.getElementById('status').textContent = message;
            setTimeout(() => {
                document.getElementById('status').textContent = 'Sistema conectado e pronto!';
            }, 3000);
        }

        // Simulação de comunicação com C++
        function sendCommand(command, data) {
            console.log('Comando enviado:', command, data);
            
            // Aqui você integraria com sua comunicação serial/WebSocket/HTTP
            // Exemplo de estrutura do comando:
            const commandData = {
                timestamp: new Date().toISOString(),
                command: command,
                payload: data
            };
            
            // Para integração real, você pode usar:
            // - WebSocket para comunicação em tempo real
            // - Fetch API para HTTP requests
            // - Serial API (em navegadores compatíveis)
            
            console.log('JSON para C++:', JSON.stringify(commandData));
        }

        // Inicialização
        document.addEventListener('DOMContentLoaded', function() {
            updateWaterLevel();
            updateStatus('Sistema carregado. Conecte ao Arduino para começar.');
            
            // Auto-conectar se já houver uma conexão serial salva
            if ('serial' in navigator) {
                navigator.serial.getPorts().then(ports => {
                    if (ports.length > 0) {
                        updateStatus('Porta serial detectada. Clique em CONECTAR.');
                    }
                });
            }
        });

        // Simular recebimento de dados do C++
        function simulateDataFromArduino() {
            // Aqui você receberia dados do seu sistema C++
            // Exemplo: status dos sensores, confirmações de comando, etc.
        }
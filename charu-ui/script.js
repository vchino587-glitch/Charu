const chatWindow = document.getElementById('chat-window');
const userInput = document.getElementById('user-input');
const sendBtn = document.getElementById('send-btn');
const debugStatus = document.getElementById('debug-status');

// --- CONFIGURACIÓN ---
// ¡Pon aquí tu URL real de Fly.io!
// Esta dirección debe apuntar a tu servidor C++.
const API_URL = 'https://charu.fly.dev'; // <--- ASEGÚRATE QUE ESTO SEA CORRECTO

debugStatus.textContent = `Conectando a: ${API_URL}`;

// Función para añadir un mensaje al chat
function addMessage(sender, text) {
    const messageDiv = document.createElement('div');
    messageDiv.classList.add('message', sender);
    
    const contentDiv = document.createElement('div');
    contentDiv.classList.add('message-content');
    contentDiv.textContent = text;
    
    messageDiv.appendChild(contentDiv);
    chatWindow.appendChild(messageDiv);
    
    // Auto-scroll hacia abajo
    chatWindow.scrollTop = chatWindow.scrollHeight;
}

// Función para enviar el mensaje al servidor
async function sendMessage() {
    const text = userInput.value.trim();
    if (!text) return;

    // 1. Mostrar el mensaje del usuario en la interfaz
    addMessage('user', text);
    userInput.value = '';

    // 2. Enviar el mensaje al servidor C++
    // Suponemos que tu servidor espera un JSON en un POST a /chat
    // Ejemplo: { "message": "¿Cómo estás?" }
    
    debugStatus.textContent = 'Enviando mensaje...';

    try {
        const response = await fetch(`${API_URL}/chat`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ message: text })
        });

        if (!response.ok) {
            throw new Error(`Error del servidor: ${response.status}`);
        }

        // 3. Recibir la respuesta del servidor
        const data = await response.json();
        // Suponemos que el servidor responde con { "response": "¡Estoy bien, Javi!" }
        addMessage('bot', data.response);
        debugStatus.textContent = 'Respuesta recibida correctamente.';

    } catch (error) {
        console.error('Error:', error);
        addMessage('bot', '¡Ups! Parece que no puedo conectarme con Charu en el servidor. Revisa la consola para ver el error.');
        debugStatus.textContent = `Error de conexión: ${error.message}`;
    }
}

// Event Listeners
sendBtn.addEventListener('click', sendMessage);
userInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        sendMessage();
    }
});
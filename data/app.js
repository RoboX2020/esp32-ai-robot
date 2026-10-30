// ============================================================================
// app.js - the phone remote control for the ESP32 robot. Runs in the
// browser (served by the robot over Wi-Fi): type text for it to show, tap
// emotion buttons to change its face, or use the microphone to talk to it
// (speech is sent to the AI backend, the reply is shown/spoken back).
// Set ESP_IP to the robot's address (shown on serial at boot).
// ============================================================================
const ESP_IP = "";

// --- DOM ELEMENTS ---
const micBtn = document.getElementById('mic-btn');
const micStatus = document.getElementById('mic-status');
const textInput = document.getElementById('text-input');
const sendBtn = document.getElementById('send-btn');
const conversationLog = document.getElementById('conversation-log');
const timeDisplay = document.getElementById('time-display');
const connectionStatus = document.getElementById('connection-status');
const settingsPanel = document.getElementById('settings-panel');
const settingsBtn = document.getElementById('settings-btn');
const closeSettingsBtn = document.getElementById('close-settings');
const apiKeyInput = document.getElementById('api-key-input');
const saveKeyBtn = document.getElementById('save-key-btn');
const fileInput = document.getElementById('file-input');
const uploadBtn = document.getElementById('upload-btn');
const promptModeToggle = document.getElementById('prompt-mode-toggle');
const gameControlsDiv = document.getElementById('game-controls');

// --- STATE ---
let isListening = false;
let isPromptMode = false;
const HARDCODED_KEY = ""; // SECURITY: API Key removed for GitHub. Enter via UI.
let openaiKey = localStorage.getItem('openai_key') || HARDCODED_KEY;
if (openaiKey) apiKeyInput.value = openaiKey;

// Game State
let gameLoopId = null;
let lastFrameTime = 0;
const FRAME_DELAY = 200; // Limit ESP updates (~5 FPS)
const gameCanvas = document.createElement('canvas'); // Virtual
gameCanvas.width = 128;
gameCanvas.height = 64;
const gameCtx = gameCanvas.getContext('2d');
let gameInputs = { up: false, down: false, left: false, right: false, a: false, b: false };


// --- SPEECH RECOGNITION ---
const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
const recognition = new SpeechRecognition();
recognition.continuous = false;
recognition.lang = 'en-US';

recognition.onresult = (event) => {
    const text = event.results[0][0].transcript;
    processInput(text);
    isListening = false;
    micBtn.classList.remove('listening');
    micStatus.textContent = "[OFF]";
};

recognition.onerror = (event) => {
    console.error("Speech Err:", event.error);
    isListening = false;
    micBtn.classList.remove('listening');
    micStatus.textContent = "[ERR]";

    if (event.error === 'not-allowed' || event.error === 'service-not-allowed') {
        addMessage('SYSTEM', "MIC BLOCKED. Check Security Settings below.");
        document.getElementById('stt-warning').style.display = 'block';
    }
};

function toggleListening() {
    if (isListening) {
        recognition.stop();
        isListening = false;
        micBtn.classList.remove('listening');
        micStatus.textContent = "[OFF]";
    } else {
        try {
            recognition.start();
            isListening = true;
            micBtn.classList.add('listening');
            micStatus.textContent = "[REC]";
        } catch (e) {
            console.error(e);
            isListening = false;
        }
    }
}


// --- EVENT LISTENERS ---
micBtn.addEventListener('click', toggleListening);
sendBtn.addEventListener('click', handleTextSubmit);
textInput.addEventListener('keypress', (e) => { if (e.key === 'Enter') handleTextSubmit(); });

settingsBtn.addEventListener('click', () => settingsPanel.classList.remove('hidden'));
closeSettingsBtn.addEventListener('click', () => settingsPanel.classList.add('hidden'));

saveKeyBtn.addEventListener('click', () => {
    openaiKey = apiKeyInput.value.trim();
    localStorage.setItem('openai_key', openaiKey);
    addMessage('SYSTEM', "API KEY SAVED.");
    settingsPanel.classList.add('hidden');
});

uploadBtn.addEventListener('click', () => fileInput.click());
fileInput.addEventListener('change', handleFileUpload);

const captionModeToggle = document.getElementById('caption-mode-toggle');
const sttWarning = document.getElementById('stt-warning');
let isCaptionMode = false;

// Check Protocol for STT
if (window.location.protocol === 'http:' && window.location.hostname !== 'localhost') {
    sttWarning.style.display = 'block';
}

promptModeToggle.addEventListener('change', (e) => {
    isPromptMode = e.target.checked;
    addMessage('SYSTEM', isPromptMode ? "MODE: CHAT (NO EYES)" : "MODE: NORMAL");
    if (isPromptMode) sendToESP('/api/display/text', { text: "CHAT MODE", persistent: true });
});

captionModeToggle.addEventListener('change', (e) => {
    isCaptionMode = e.target.checked;
    addMessage('SYSTEM', isCaptionMode ? "MODE: CAPTION (One-Word)" : "MODE: FULL TEXT");
});

// Game Controls Binding
const bindBtn = (id, key) => {
    const btn = document.getElementById(id);
    if (btn) {
        btn.addEventListener('mousedown', () => gameInputs[key] = true);
        btn.addEventListener('mouseup', () => gameInputs[key] = false);
        btn.addEventListener('touchstart', (e) => { e.preventDefault(); gameInputs[key] = true; });
        btn.addEventListener('touchend', (e) => { e.preventDefault(); gameInputs[key] = false; });
    }
};
bindBtn('btn-up', 'up'); bindBtn('btn-down', 'down');
bindBtn('btn-left', 'left'); bindBtn('btn-right', 'right');
bindBtn('btn-a', 'a'); bindBtn('btn-b', 'b');

// Keyboard Support
document.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowUp') gameInputs.up = true;
    if (e.key === 'ArrowDown') gameInputs.down = true;
    if (e.key === 'ArrowLeft') gameInputs.left = true;
    if (e.key === 'ArrowRight') gameInputs.right = true;
    if (e.key === 'z') gameInputs.a = true;
    if (e.key === 'x') gameInputs.b = true;
});
document.addEventListener('keyup', (e) => {
    if (e.key === 'ArrowUp') gameInputs.up = false;
    if (e.key === 'ArrowDown') gameInputs.down = false;
    if (e.key === 'ArrowLeft') gameInputs.left = false;
    if (e.key === 'ArrowRight') gameInputs.right = false;
    if (e.key === 'z') gameInputs.a = false;
    if (e.key === 'x') gameInputs.b = false;
});


// --- LOOPS ---
setInterval(updateTime, 1000);
updateTime();


// --- CORE FUNCTIONS ---

function handleTextSubmit() {
    const text = textInput.value.trim();
    if (!text) return;
    processInput(text);
    textInput.value = "";
}

function handleFileUpload(e) {
    const file = e.target.files[0];
    if (!file) return;

    addMessage('SYSTEM', "PROCESSING IMG...");

    const reader = new FileReader();
    reader.onload = function (event) {
        const img = new Image();
        img.onload = function () {
            // Resize to 128x64
            const canvas = document.createElement('canvas');
            canvas.width = 128;
            canvas.height = 64;
            const ctx = canvas.getContext('2d');
            ctx.drawImage(img, 0, 0, 128, 64);

            // Get Pixels and threshold
            const imageData = ctx.getImageData(0, 0, 128, 64);
            const data = imageData.data;
            let binaryString = "";
            for (let i = 0; i < data.length; i += 4) {
                const avg = (data[i] + data[i + 1] + data[i + 2]) / 3;
                binaryString += (avg > 128) ? "1" : "0";
            }
            sendToESP('/api/display/image', { bitmap: binaryString });
            addMessage('ROBOT', "IMAGE UPLOADED.");
        }
        img.src = event.target.result;
    }
    reader.readAsDataURL(file);
}

async function processInput(text) {
    addMessage('USER', text);

    // In Conversation Mode, show Question on OLED immediately
    if (isPromptMode) {
        await sendToESP('/api/display/text', { text: "Q: " + text, persistent: true });
    }

    if (!openaiKey) {
        addMessage('ROBOT', "ERROR: MISSING API KEY.");
        speak("Please configure API key.");
        return;
    }

    addMessage('SYSTEM', "PROCESSING...");
    const aiResponse = await fetchOpenAI(text);

    if (aiResponse) {
        const aiText = aiResponse.text;

        // 0. CHECK FOR THINKING BLOCK
        const thinkingMatch = aiText.match(/<thinking>([\s\S]*?)<\/thinking>/);
        if (thinkingMatch) {
            const thought = thinkingMatch[1].trim();
            addMessage('SYSTEM', "🧠 THOUGHT: " + thought);
            // We could speak this too: speak("I am thinking about..." + thought.substring(0, 50));
        }

        // 1. CHECK FOR CODE (Game/Animation)
        const codeBlock = aiText.match(/```javascript([\s\S]*?)```/) || aiText.match(/```([\s\S]*?)```/);

        if (codeBlock || aiText.includes("ctx.")) {
            addMessage('ROBOT', "Compiling Game/Animation...");
            const code = codeBlock ? codeBlock[1] : aiText;

            try {
                if (gameLoopId) clearInterval(gameLoopId);

                const gameState = {}; // Persistent state

                // Audio Helper for Sandbox
                const playTone = (freq, duration) => {
                    sendToESP('/api/sound/tone', { freq, duration });
                };

                const updateFunc = new Function('ctx', 'width', 'height', 'inputs', 'state', 'time', 'playTone', code);

                gameControlsDiv.classList.remove('hidden');

                gameLoopId = setInterval(async () => {
                    const now = Date.now();
                    // Clear
                    gameCtx.fillStyle = 'black';
                    gameCtx.fillRect(0, 0, 128, 64);
                    gameCtx.fillStyle = 'white';

                    try {
                        updateFunc(gameCtx, 128, 64, gameInputs, gameState, now, playTone);

                        // Stream to ESP
                        if (now - lastFrameTime > FRAME_DELAY) {
                            lastFrameTime = now;
                            const imageData = gameCtx.getImageData(0, 0, 128, 64);
                            const data = imageData.data;
                            let binaryString = "";
                            for (let i = 0; i < data.length; i += 4) {
                                const avg = (data[i] + data[i + 1] + data[i + 2]) / 3;
                                binaryString += (avg > 128) ? "1" : "0";
                            }
                            await sendToESP('/api/display/image', { bitmap: binaryString });
                        }
                    } catch (err) {
                        console.error("Game Loop Err:", err);
                        clearInterval(gameLoopId);
                        addMessage('SYSTEM', "Game Crashed: " + err.message);
                    }
                }, 50);

                addMessage('ROBOT', "Running Game/Animation!");
            } catch (e) {
                addMessage('SYSTEM', "Compile Error: " + e.message);
            }
            return;
        }

        // 2. NORMAL RESPONSE
        addMessage('ROBOT', aiText);
        speak(aiText);

        // Display Answer on OLED
        let cleanText = aiText.replace(/\n/g, " ");

        if (isCaptionMode) {
            // CAPTION MODE: One word at a time
            const words = cleanText.split(' ');
            let i = 0;
            const captionInterval = setInterval(async () => {
                if (i >= words.length) {
                    clearInterval(captionInterval);
                    // After done, clear or stay?
                    if (!isPromptMode && !isCaptionMode) setEmotion("neutral");
                    return;
                }

                let word = words[i];
                // Group short words? No, strictly one word requested.
                // Center it by adding padding? OLED logic handles centering for short text.

                // If Chat Mode is ALSO on, we want to keep the text? No, Caption flows.
                // We send with persistent=true so it stays until next word replaces it
                await sendToESP('/api/display/text', { text: word, persistent: true });
                i++;
            }, 400); // 400ms per word (RSVT speed)

        } else {
            // STANDARD DISPLAY
            await sendToESP('/api/display/text', {
                text: cleanText,
                persistent: isPromptMode // If Chat Mode is ON, text stays.
            });
        }

        // Emotion Logic (Only if NOT in Prompt Mode)
        // If Caption Mode is ON, we probably want neutral eyes or just text?
        // If Caption is ON, eyes are replaced by text anyway.
        if (!isPromptMode && !isCaptionMode) {
            const lower = aiText.toLowerCase();
            if (lower.includes("happy") || lower.includes("joke")) setEmotion("happy");
            else if (lower.includes("sad") || lower.includes("sorry")) setEmotion("sad");
            else if (lower.includes("angry")) setEmotion("angry");
            else if (lower.includes("spider")) setEmotion("spiderman");
            else setEmotion("neutral");
        }
    }
}

async function fetchOpenAI(prompt) {
    // System Prompt for Super Agent with Thinking Layer
    const systemPrompt = `You are a witty retro robot AI for a 128x64 OLED screen. 
    1. If asked to CREATE A GAME, ANIMATION, or DRAWING:
       - FIRST, output a <thinking> block to plan your logic, physics, variables, and edge cases. Take your time to design a robust system.
       - THEN, output ONLY a Javascript code block starting with \`\`\`javascript. 
       - This code is the BODY of an 'update(ctx, width, height, inputs, state, time)' function.
       - 'ctx': Canvas 2D Context.
       - 'inputs': {up, down, left, right, a, b} (booleans).
       - 'state': Persistent object across frames. Initialize variables in state.
       - 'time': Current timestamp (ms).
       - 'playTone(freq, duration)': Function to play sound. freq in Hz, duration in ms. Example: playTone(440, 100);
       - Draw with white (fillStyle='white'). Background is already black.
       - Do NOT use requestAnimationFrame or loops, just the frame logic.
    2. If normal chat, keep it short.
    3. If user toggles 'PROMPT MODE', just answer questions.`;

    try {
        const res = await fetch('https://api.openai.com/v1/chat/completions', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json', 'Authorization': `Bearer ${openaiKey}` },
            body: JSON.stringify({
                model: "gpt-4o",
                messages: [
                    { role: "system", content: systemPrompt },
                    { role: "user", content: prompt }
                ]
            })
        });
        const data = await res.json();
        if (data.error) throw new Error(data.error.message);
        return { text: data.choices[0].message.content };
    } catch (e) {
        addMessage('SYSTEM', "ERR: " + e.message);
        return null;
    }
}

function addMessage(sender, text) {
    const div = document.createElement('div');
    div.classList.add('message', sender.toLowerCase());
    div.innerHTML = `<span class="sender">[${sender}]:</span> <span class="text">${text}</span>`;
    conversationLog.appendChild(div);
    conversationLog.scrollTop = conversationLog.scrollHeight;
}

function speak(text) {
    if ('speechSynthesis' in window) {
        const u = new SpeechSynthesisUtterance(text);
        u.rate = 1.0;
        window.speechSynthesis.speak(u);
    }
}

async function sendToESP(ep, data) {
    try {
        await fetch(ep, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
    } catch (e) { }
}

function setEmotion(em) {
    sendToESP('/api/eyes/emotion', { emotion: em });
}

function updateTime() {
    const d = new Date();
    timeDisplay.textContent = d.toLocaleTimeString('en-US', { hour12: false });

    // Sync to ESP every minute
    if (d.getSeconds() === 0) {
        const timeStr = d.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
        const dateStr = d.toLocaleDateString('en-US');
        sendToESP('/api/display/text', { text: `CMD_TIME:${timeStr}|${dateStr}` });
    }
}

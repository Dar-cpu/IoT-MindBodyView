const express = require('express');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 3000;
const DATA_FILE = path.join(__dirname, 'data.json');
const COMMAND_FILE = path.join(__dirname, 'command.json');

app.use(express.json());
app.use(cors());

// Inicializar archivos
if (!fs.existsSync(DATA_FILE)) {
    fs.writeFileSync(DATA_FILE, JSON.stringify({}));
}

if (!fs.existsSync(COMMAND_FILE)) {
    fs.writeFileSync(COMMAND_FILE, JSON.stringify({ command: '' }));
}

// Endpoint para recibir datos del ESP
app.post('/data', (req, res) => {
    const newData = req.body;
    
    try {
        // Leer datos existentes
        const rawData = fs.readFileSync(DATA_FILE);
        const data = JSON.parse(rawData);
        
        // Actualizar con nuevos datos
        const timestamp = new Date().toISOString();
        const updatedData = {
            ...data,
            [timestamp]: newData
        };

        fs.writeFileSync(DATA_FILE, JSON.stringify(updatedData, null, 2));

        const commandData = JSON.parse(fs.readFileSync(COMMAND_FILE));
        const response = commandData.command ? { command: commandData.command } : { status: 'Datos recibidos' };

        if (commandData.command) {
            fs.writeFileSync(COMMAND_FILE, JSON.stringify({ command: '' }));
        }
        
        res.json(response);
    } catch (error) {
        console.error('Error POST /data:', error);
        res.status(500).send('Error procesando datos');
    }
});

// Endpoint para que el frontend obtenga datos
app.get('/data', (req, res) => {
    try {
        const rawData = fs.readFileSync(DATA_FILE);
        res.header('Content-Type', 'application/json');
        res.send(rawData);
    } catch (error) {
        console.error('Error GET /data:', error);
        res.status(500).send('Error leyendo datos');
    }
});

// Endpoint para recibir comandos del frontend
app.post('/command', (req, res) => {
    const { command } = req.body;
    
    if (!command) {
        return res.status(400).send('Comando requerido');
    }
    
    try {
        fs.writeFileSync(COMMAND_FILE, JSON.stringify({ command }));
        res.send('Comando recibido');
    } catch (error) {
        console.error('Error POST /command:', error);
        res.status(500).send('Error guardando comando');
    }
});

app.get('/command', (req, res) => {
    try {
        const commandData = JSON.parse(fs.readFileSync(COMMAND_FILE)); // Algunas funcionalidades no estan del todoimplementadas 
        res.json({ command: commandData.command });
    } catch (error) {
        console.error('Error GET /command:', error);
        res.status(500).send('Error leyendo comando');
    }
});

// Iniciar servidor
app.listen(PORT, () => {
    console.log(`Servidor ejecutándose en http://localhost:${PORT}`);
    console.log(`ESP32 debe enviar datos a: http://192.168.100.3:${PORT}/data`);
});
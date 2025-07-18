/* eslint-disable react-hooks/exhaustive-deps */
import './index.css';
import React, { useState, useEffect } from 'react';
import { 
  Trash2, 
  Thermometer, 
  Droplets, 
  Flame, 
  Users, 
  Battery, 
  Zap, 
  AlertTriangle, 
  CheckCircle, 
  Coins,
  TrendingUp,
  Recycle
} from 'lucide-react';

interface SensorData {
  trash: number;
  temp: number;
  hum: number;
  flame: boolean;
  bat: number;
  tokens: number;
  deps: number;
  win: boolean;
  button: boolean;
  time: number;
}

function App() {
  const [sensorData, setSensorData] = useState<SensorData>({
    trash: 50,
    temp: 22,
    hum: 66,
    flame: false,
    bat: 0,
    tokens: 0,
    deps: 0,
    win: false,
    button: false,
    time: 230
  });

  const [alerts, setAlerts] = useState<string[]>([]);
  const [buttonPressed, setButtonPressed] = useState(false);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await fetch('http://localhost:3000/data');
        const allData = await res.json();
        const lastKey = Object.keys(allData).sort().pop();
        if (lastKey) {
          setSensorData(allData[lastKey]);
        }
      } catch (err) {
        console.error('Error al obtener datos del backend:', err);
      }
    };

    fetchData(); // Llamada inicial
    const interval = setInterval(fetchData, 5000); // Cada 5 segundos

    return () => clearInterval(interval);
  }, []);
 
  // Handle alerts
  useEffect(() => {
    const newAlerts: string[] = [];
    
    if (sensorData.trash > 85) {
      newAlerts.push('Contenedor casi lleno');
    }
    if (sensorData.flame) {
      newAlerts.push('¡ALERTA DE FUEGO DETECTADA!');
    }
    if (sensorData.bat < 20) {
      newAlerts.push('Batería baja - Verificar carga');
    }
    
    setAlerts(newAlerts);
  }, [sensorData]);

  // Handle button state from ESP32
  useEffect(() => {
    if (sensorData.button && !buttonPressed) {
      setButtonPressed(true);
      setTimeout(() => setButtonPressed(false), 5000);
    }
  }, [sensorData.button]);
  
  const handleDeposit = () => {
    setSensorData(prev => ({
      ...prev,
      tokens: prev.tokens + 5,
      deps: prev.deps + 1,
      trash: Math.min(100, prev.trash + 3)
    }));
  };

  const getStatusColor = (level: number, inverse = false) => {
    if (inverse) {
      if (level > 70) return 'text-red-500';
      if (level > 40) return 'text-yellow-500';
      return 'text-green-500';
    }
    if (level < 30) return 'text-red-500';
    if (level < 60) return 'text-yellow-500';
    return 'text-green-500';
  };

  // Determinar si está cargando con energía de la red
  const isCharging = sensorData.bat > 0 && sensorData.bat < 100;

  return (
    <div className="min-h-screen bg-gradient-to-br from-emerald-50 to-blue-50 p-4">
      <div className="max-w-6xl mx-auto">
        {/* Header */}
        <div className="bg-white rounded-xl shadow-lg p-6 mb-6">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <div className="p-3 bg-emerald-100 rounded-full">
                <Recycle className="w-8 h-8 text-emerald-600" />
              </div>
              <div>
                <h1 className="text-2xl font-bold text-gray-800">EcoSmart Container</h1>
                <p className="text-gray-600">Sistema Inteligente de Gestión de Residuos</p>
              </div>
            </div>
            
            <div className="flex items-center space-x-4">
              <div className="flex items-center space-x-2">
                <Battery className={`w-5 h-5 ${getStatusColor(sensorData.bat)}`} />
                <span className="font-semibold">{sensorData.bat}%</span>
              </div>
              {isCharging && (
                <div className="flex items-center space-x-2 text-blue-500">
                  <Zap className="w-5 h-5" />
                  <span className="text-sm">Cargando</span>
                </div>
              )}
            </div>
          </div>
        </div>

        {/* Alerts */}
        {alerts.length > 0 && (
          <div className="bg-red-50 border-l-4 border-red-500 p-4 mb-6 rounded-r-xl">
            <div className="flex items-center">
              <AlertTriangle className="w-5 h-5 text-red-500 mr-2" />
              <h3 className="text-red-800 font-semibold">Alertas Activas</h3>
            </div>
            <ul className="mt-2 text-red-700">
              {alerts.map((alert, index) => (
                <li key={index} className="text-sm">• {alert}</li>
              ))}
            </ul>
          </div>
        )}

        {/* Main Grid */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Left Column - Sensors */}
          <div className="lg:col-span-2 space-y-6">
            {/* Trash Level */}
            <div className="bg-white rounded-xl shadow-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="flex items-center space-x-3">
                  <Trash2 className="w-6 h-6 text-gray-600" />
                  <h2 className="text-xl font-semibold text-gray-800">Nivel de Basura</h2>
                </div>
              </div>
              
              <div className="relative">
                <div className="w-full bg-gray-200 rounded-full h-8">
                  <div 
                    className={`h-8 rounded-full transition-all duration-500 ${
                      sensorData.trash > 80 ? 'bg-red-500' :
                      sensorData.trash > 60 ? 'bg-yellow-500' : 'bg-green-500'
                    }`}
                    style={{ width: `${sensorData.trash}%` }}
                  ></div>
                </div>
                <span className="absolute inset-0 flex items-center justify-center text-white font-bold">
                  {sensorData.trash}%
                </span>
              </div>
            </div>

            {/* Environmental Sensors */}
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              {/* Temperature & Humidity */}
              <div className="bg-white rounded-xl shadow-lg p-6">
                <h3 className="text-lg font-semibold text-gray-800 mb-4">Ambiente</h3>
                <div className="space-y-3">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <Thermometer className="w-5 h-5 text-red-500" />
                      <span className="text-gray-600">Temperatura</span>
                    </div>
                    <span className="font-bold text-red-500">{sensorData.temp}°C</span>
                  </div>
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <Droplets className="w-5 h-5 text-blue-500" />
                      <span className="text-gray-600">Humedad</span>
                    </div>
                    <span className="font-bold text-blue-500">{sensorData.hum}%</span>
                  </div>
                </div>
              </div>

              {/* Safety Sensors */}
              <div className="bg-white rounded-xl shadow-lg p-6">
                <h3 className="text-lg font-semibold text-gray-800 mb-4">Seguridad</h3>
                <div className="space-y-3">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <Flame className="w-5 h-5 text-orange-500" />
                      <span className="text-gray-600">Llama</span>
                    </div>
                    <div className={`flex items-center ${sensorData.flame ? 'text-red-500' : 'text-green-500'}`}>
                      {sensorData.flame ? (
                        <AlertTriangle className="w-5 h-5" />
                      ) : (
                        <CheckCircle className="w-5 h-5" />
                      )}
                      <span className="ml-1 font-bold">
                        {sensorData.flame ? 'Detectada' : 'Normal'}
                      </span>
                    </div>
                  </div>
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <Battery className="w-5 h-5 text-green-500" />
                      <span className="text-gray-600">Batería</span>
                    </div>
                    <span className={`font-bold ${getStatusColor(sensorData.bat)}`}>
                      {sensorData.bat}%
                    </span>
                  </div>
                </div>
              </div>
            </div>

            {/* Button Detection */}
            <div className="bg-white rounded-xl shadow-lg p-6">
              <div className="flex items-center justify-between mb-4">
                <div className="flex items-center space-x-3">
                  <Users className="w-6 h-6 text-purple-600" />
                  <h2 className="text-xl font-semibold text-gray-800">Detección de Usuario</h2>
                </div>
                <div className={`px-3 py-1 rounded-full text-sm font-semibold ${
                  sensorData.button 
                    ? 'bg-green-100 text-green-800' 
                    : 'bg-gray-100 text-gray-600'
                }`}>
                  {sensorData.button ? 'Botón Activo' : 'Esperando'}
                </div>
              </div>
              
              {sensorData.button && (
                <div className="p-4 bg-green-50 rounded-lg border border-green-200">
                  <div className="flex items-center justify-between">
                    <span className="text-green-800 font-semibold">¡Botón presionado! Sistema activado</span>
                    <div className="flex items-center text-green-600">
                      <div className="w-2 h-2 bg-green-500 rounded-full mr-2 animate-pulse"></div>
                      <span className="text-sm font-semibold">Activo</span>
                    </div>
                  </div>
                </div>
              )}
              
              {sensorData.win && (
                <div className="p-4 bg-blue-50 rounded-lg border border-blue-200 mt-4">
                  <div className="flex items-center justify-between">
                    <span className="text-blue-800 font-semibold">¡Ventana Abierta! Deposite su basura</span>
                    <button
                      onClick={handleDeposit}
                      className="px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 transition-colors"
                    >
                      Confirmar Depósito
                    </button>
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Right Column - Rewards & Stats */}
          <div className="space-y-6">
            {/* Rewards */}
            <div className="bg-gradient-to-br from-yellow-400 to-orange-500 rounded-xl shadow-lg p-6 text-white">
              <div className="flex items-center space-x-3 mb-4">
                <Coins className="w-8 h-8" />
                <h2 className="text-xl font-bold">Tokens EcoSmart</h2>
              </div>
              <div className="text-center">
                <div className="text-4xl font-bold mb-2">{sensorData.tokens}</div>
                <p className="text-yellow-100">Tokens Disponibles</p>
              </div>
              <div className="mt-4 p-3 bg-white/20 rounded-lg">
                <p className="text-sm text-yellow-100">
                  Cada depósito de basura te otorga 5 tokens que puedes canjear por premios ecológicos
                </p>
              </div>
            </div>

            {/* Daily Stats */}
            <div className="bg-white rounded-xl shadow-lg p-6">
              <div className="flex items-center space-x-3 mb-4">
                <TrendingUp className="w-6 h-6 text-green-600" />
                <h2 className="text-xl font-semibold text-gray-800">Estadísticas del Día</h2>
              </div>
              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Depósitos realizados</span>
                  <span className="font-bold text-green-500">{sensorData.deps}</span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Tokens ganados hoy</span>
                  <span className="font-bold text-yellow-500">{sensorData.deps * 5}</span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Impacto ambiental</span>
                  <span className="font-bold text-blue-500">+{(sensorData.deps * 0.1).toFixed(1)} kg CO₂</span>
                </div>
              </div>
            </div>

            {/* System Status */}
            <div className="bg-white rounded-xl shadow-lg p-6">
              <h2 className="text-xl font-semibold text-gray-800 mb-4">Estado del Sistema</h2>
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Conectividad</span>
                  <div className="flex items-center text-green-500">
                    <div className="w-2 h-2 bg-green-500 rounded-full mr-2"></div>
                    <span className="text-sm font-semibold">Online</span>
                  </div>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Botón de Usuario</span>
                  <div className={`flex items-center ${sensorData.button ? 'text-green-500' : 'text-gray-400'}`}>
                    <div className={`w-2 h-2 rounded-full mr-2 ${sensorData.button ? 'bg-green-500' : 'bg-gray-400'}`}></div>
                    <span className="text-sm font-semibold">{sensorData.button ? 'Presionado' : 'Inactivo'}</span>
                  </div>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Ventana de Depósito</span>
                  <div className={`flex items-center ${sensorData.win ? 'text-green-500' : 'text-gray-400'}`}>
                    <div className={`w-2 h-2 rounded-full mr-2 ${sensorData.win ? 'bg-green-500' : 'bg-gray-400'}`}></div>
                    <span className="text-sm font-semibold">{sensorData.win ? 'Abierta' : 'Cerrada'}</span>
                  </div>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-gray-600">Tiempo de funcionamiento</span>
                  <span className="text-sm font-semibold text-gray-600">
                    {Math.floor(sensorData.time / 60)}m {sensorData.time % 60}s
                  </span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
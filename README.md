# Chrome200

> **Estado: en desarrollo activo.** Esta versión de **CHROME 2000 Sample Synth** todavía no es una versión final y su interfaz está pendiente de una reconstrucción manual a partir del diseño definitivo de Figma.

## Proyecto propietario — no es open source

Aunque este repositorio sea visible públicamente durante la etapa de desarrollo, **CHROME 2000 no es un proyecto de código abierto**. La publicación temporal del código no concede permiso para copiar, distribuir, modificar, sublicenciar, revender ni reutilizar total o parcialmente el producto, sus recursos o sus diseños.

Todo el contenido está protegido por la licencia propietaria incluida en [LICENSE.md](LICENSE.md). Cuando termine el desarrollo, el repositorio dejará de ser público.

## Descripción

CHROME 2000 es un instrumento VST3/Standalone para Windows basado en tres fuentes de audio y una arquitectura híbrida de cinta, transformación espectral y microgranularidad. El proyecto usa C++20, JUCE 8 y CMake, y está preparado para Visual Studio 2022 x64.

## Compilación rápida en Windows 10/11

Requisitos:

- Visual Studio 2022 con la carga **Desktop development with C++** y Windows 10/11 SDK.
- CMake 3.24 o posterior (incluido por Visual Studio si se marca *C++ CMake tools for Windows*).
- Git, únicamente si CMake debe descargar JUCE.

Desde **Developer PowerShell for VS 2022**:

```powershell
cd "C:\ruta\a\Chrome2000"
.\Build-Windows.ps1 -Configuration Release
```

La primera configuración descarga JUCE 8.0.12. Para trabajar sin red, descargue JUCE 8 y use:

```powershell
.\Build-Windows.ps1 -Configuration Release -JucePath "C:\SDK\JUCE"
```

También puede abrir la carpeta directamente con Visual Studio 2022; Visual Studio detectará `CMakePresets.json`. Seleccione `vs2022-x64`, configure y compile `Chrome2000_VST3` o `Chrome2000_Standalone`.

El VST3 se genera en `build/vs2022-x64/Chrome2000_artefacts/Release/VST3/CHROME 2000.vst3`. Cópielo a `%COMMONPROGRAMFILES%\VST3`, inicie FL Studio 25 y ejecute **Find installed plugins**.

## Flujo de uso

1. Arrastre WAV, AIFF o FLAC a cualquiera de los tres slots. Se aceptan archivos mono/estéreo, PCM de hasta 24 bits y float de 32 bits mediante los lectores de JUCE.
2. Ajuste inicio/fin, loop, reverse, afinación, panorama y ganancia por fuente.
3. Seleccione por slot `Vintage`, `Chrome` o `Digital`.
4. Use Morph X/Y para ponderar las tres fuentes y los seis controles Chrome para definir la transformación.
5. Guarde el preset desde la barra superior. Los presets de usuario se almacenan en `%APPDATA%\LEVSAN\CHROME 2000\Presets` y conservan parámetros y rutas de las tres muestras.

## Arquitectura de audio

- `SampleLoader`: decodificación en hilo de baja prioridad y publicación atómica de audio inmutable.
- `Voice`: reproducción interpolada, ADSR, velocidad, loop/reverse y panorama; hasta 128 instancias preasignadas.
- `SampleEngine`: planificación MIDI por offsets exactos, mezcla de tres buses y robo de la voz más antigua.
- `VintageProcessor`: saturación suave, compresión dependiente de envolvente, head bump, wow/flutter, ruido y roll-off.
- `ChromeProcessor`: memoria espectral FFT parcial, ocho resonadores modales inarmónicos, microfragmentación adaptativa, red de cuatro all-pass, comb dinámico y brillo por micro-frequency modulation.
- `DigitalProcessor`: nube de microcortes con densidad y textura variables.
- `MasterEffects`: filtro multimodo 12/24 dB, chorus, delay, reverb, compresor, EQ, anchura M/S, soft clip y limitador.

Las voces, buffers de buses, FFT, delays y efectos se asignan en `prepareToPlay`; el callback no crea buffers ni realiza E/S. La carga de archivos y las notificaciones de GUI están fuera del hilo de audio.

## Automatización y estado

Todos los controles de síntesis y efectos se publican mediante `AudioProcessorValueTreeState`, incluyendo tres LFO, ocho rutas de matriz de modulación, polifonía, oversampling y power. El host recibe identificadores estables versionados. El estado de sesión VST3 incluye las rutas de muestras y todos los parámetros.

## Presets incluidos

- Init Patch
- Liquid Cathedral
- Mercury Glass
- Tape Constellation
- Digital Pollen
- Frozen Alloy

Los presets de fábrica son independientes de archivos externos; adquieren su carácter al cargar cualquier material en uno o más slots. Esto evita distribuir audio de terceros y mantiene sesiones portables cuando las muestras del usuario permanecen en su ubicación original.

## Notas de rendimiento

Para equipos antiguos se recomienda 32 voces y bloques de 256–512 muestras. Chrome usa FFT de 1024 puntos por canal/slot y buffers fijos. La opción Standalone permite probar MIDI y audio sin un DAW. Las compilaciones Release activan LTO y las optimizaciones recomendadas por JUCE.

# Arquitectura técnica

El procesador del plugin posee el estado automatizable, el motor de muestras y el gestor de presets. El editor sólo observa esos servicios; cerrar la interfaz no altera el audio.

## Reglas de tiempo real

- No se abre, lee ni decodifica ningún archivo en `processBlock`.
- Las 128 voces y todos los buffers de DSP se reservan durante `prepareToPlay`.
- La muestra cargada es inmutable. El loader publica un `shared_ptr` atómicamente y la voz conserva su referencia durante la nota.
- MIDI se renderiza por segmentos entre eventos, por lo que note-on y note-off conservan precisión de muestra.
- Los medidores comunican datos mediante atomics relajados.

## Motor Chrome

La señal de cada slot pasa por seis fenómenos acoplados, no por una reverberación única:

1. La FFT mantiene una memoria suavizada de magnitudes y la mezcla de forma parcial con el cuadro actual.
2. Un banco modal usa relaciones inarmónicas de placa/tubo y tiempos de caída dependientes de Reflect.
3. Fragment aplica eventos breves de pérdida de continuidad que excitan la red modal.
4. Cuatro retardos all-pass con longitudes coprimas producen difusión sin una cola de sala convencional.
5. Un comb móvil une Liquid y Reflect y crea reflejos dependientes de fase.
6. Shine introduce una modulación de frecuencia sub-audible y ponderación espectral ascendente.

Fusion controla la reintegración con la fuente y Density modifica excitación y realimentación. Al existir una instancia por slot, las tres texturas pueden transformarse antes del morph.

## Extensión segura

Nuevos parámetros deben añadirse al final de `ParameterLayout.cpp` con IDs permanentes. Un procesador nuevo debe reservar memoria en `prepare`, limpiar estado en `reset` y procesar in-place. Las operaciones de biblioteca, análisis largo o creación de thumbnails deben enviarse a un worker, nunca al callback.

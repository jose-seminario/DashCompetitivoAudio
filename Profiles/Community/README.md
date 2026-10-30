# DashCompetitivoAudio — Guía de Perfiles Comunitarios

Este directorio alberga perfiles comunitarios creados por jugadores y especialistas acústicos para diferentes títulos competitivos y modelos de auriculares.

## 1. Principios de Seguridad y Aceptación
- **Solo Datos JSON**: Los perfiles deben ser estrictamente archivos `.json` conforme a `Profiles/Schema/audio-profile.schema.json`.
- **Cero Código Ejecutable**: Está terminantemente prohibido incluir comandos, scripts (PowerShell, BAT, VBS), ejecutables o rutas a DLL/EXE. El validador (`ProfileValidator`) rechazará automáticamente cualquier archivo que contenga dichos patrones.
- **Límite de Tamaño**: Ningún archivo de perfil debe superar 1 MB.

## 2. Honestidad en Calibración de Auriculares
- **Verificado (`verified`)**: Solo perfiles con curvas de ecualización paramétrica derivadas de mediciones en simuladores estandarizados (Gras 43AG, BK 5128) con fuentes y licencias comprobables (ej: AutoEq de Jaakkopasanen).
- **Pendiente de Medición (`uncalibrated_pending_verified_measurement`)**: Modelos cuya respuesta de frecuencia aún no cuenta con una medición de laboratorio de acceso público validada (ej: Razer BlackShark V2 X). Nunca se deben inventar curvas acústicas.
- **Comunitario / Experimental (`community` / `experimental`)**: Ajustes empíricos realizados por usuarios para optimizar la audición personal.

## 3. Formato de Contribución
Para agregar un perfil:
1. Copia una plantilla desde `DashAudioDB/Presets/`.
2. Asigna un `profileId` seguro en kebab-case (ej: `warzone-footsteps-extreme`).
3. Completa los metadatos de autoría, licencia (se recomienda CC-BY-4.0) y changelog.
4. Valida el archivo con las pruebas de `ProfileValidator`.

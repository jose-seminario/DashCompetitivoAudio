# Avisos de Software de Terceros (Third-Party Notices)

Este documento detalla las bibliotecas, herramientas de compilación y conjuntos de datos de terceros utilizados en **Dash Competitivo Audio**, así como el cumplimiento de sus respectivas licencias.

---

## 1. Bibliotecas Enlazadas en Compilación

### JUCE Framework (v8.0.4)
- **Autor / Titular:** Raw Material Software Limited
- **Repositorio:** https://github.com/juce-framework/JUCE
- **Licencia:** **GNU General Public License v3 (GPLv3)** (modalidad de código abierto).
- **Uso:** Interfaz gráfica de usuario, gestión de mensajes y eventos, formatos de audio y bloques matemáticos DSP.

### Steinberg VST3 SDK (a través de JUCE)
- **Autor / Titular:** Steinberg Media Technologies GmbH
- **Licencia:** **GNU General Public License v3 (GPLv3)**.
- **Uso:** Estructura de plugins VST3.

---

## 2. Herramientas de Compilación y CI/CD
- **CMake (3.22+):** Kitware, Inc. — Licencia **BSD 3-Clause**.
- **NSIS — Nullsoft Scriptable Install System (3.x):** Licencia **zlib/libpng**.
- **GitHub Actions (Windows Server 2022 runner):** Plataforma de integración continua.

---

## 3. Conjuntos de Datos y Filtros Acústicos de Terceros

### AutoEq Project
- **Autor:** Jaakko Pasanen (Jaakkopasanen)
- **Repositorio:** https://github.com/jaakkopasanen/AutoEq
- **Licencia:** **The MIT License (MIT)**
  ```text
  Copyright (c) 2019-2023 Jaakko Pasanen

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  ```
- **Uso:** Coeficientes de ecualización paramétrica compensatoria para modelos HyperX Cloud II y Logitech G Pro X orientados hacia la curva target Harman Over-Ear 2018.

### Atribución de Mediciones Acústicas
- **oratory1990**: Mediciones de auriculares realizadas en simuladores auditivos estandarizados según normas IEC.

---

## 4. Dependencias del Entorno de Usuario (No Redistribuidas)
- **Equalizer APO (GPLv2)**: El software actúa como generador del archivo de texto plano administrado mediante directivas `Include`, sin enlazar con binarios de Equalizer APO.
- **VB-Audio Virtual Cable (Freeware)**: El usuario instala este controlador de forma voluntaria; DashCompetitivoAudio no fuerza ni redistribuye drivers en el instalador.

---

## 5. Exclusiones Explícitas de Código Propietario
- **Bravo Engine**: Queda expresamente excluida la incorporación de código o algoritmos propietarios del motor Bravo.
- **ArtTuneDB**: No se copian ni extraen volcados sin autorización previa verificada de licencias.

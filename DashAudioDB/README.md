# DashAudioDB — Biblioteca Acústica Comunitaria Local

**DashAudioDB** es el repositorio local de perfiles, calibraciones acústicas y ecualizaciones optimizadas para audio competitivo en Windows, diseñado para interoperar con DashCompetitivoAudio sin requerir recompilación del software ni conexiones a servidores externos.

---

## Estructura del Repositorio

```text
DashAudioDB/
├── README.md               # Este documento
├── CONTRIBUTING.md         # Normas estrictas de contribución y auditoría acústica
├── LICENSES.md             # Términos de licenciamiento (CC-BY-4.0, MIT, atribuciones)
│
├── Schema/                 # Esquemas de validación JSON formales
│   └── community-profile.schema.json
│
├── Games/                  # Perfiles optimizados por videojuego táctico
│   ├── README.md
│   └── Warzone/            # Call of Duty: Warzone (Battle Royale / Resurgence)
│       └── warzone_footsteps_focus.json
│
├── Headphones/             # Calibraciones de auriculares hacia Harman / Diffuse Field
│   └── README.md
│
├── Spatial/                # Perfiles de convolución HRTF / HRIR y ensanchamiento 3D
│   └── README.md
│
├── Presets/                # Plantillas y configuraciones base
│   ├── README.md
│   ├── flat_reference.json
│   └── tactical_footsteps.json
│
└── Contributors/           # Directorio de contribuyentes y directrices
    └── CONTRIBUTING.md
```

---

## Política de Licenciamiento
- **Datos y Contenido Propio de DashAudio**: Licenciados bajo **Creative Commons Attribution 4.0 International (CC-BY-4.0)**, permitiendo reutilización, modificación y distribución comercial manteniendo la atribución.
- **Calibraciones de Terceros (AutoEq / Oratory1990)**: Conservan íntegramente sus licencias de origen (MIT / Atribución original).
- **Prohibiciones Absolutas**: No se incorpora el motor propietario Bravo ni volcados no autorizados de bases de datos cerradas como ArtTuneDB.

---

## Requisitos de Integración
Cualquier perfil añadido a `DashAudioDB` es analizado y verificado por la suite C++ `ProfileValidator` antes de presentarse al usuario en la interfaz gráfica.

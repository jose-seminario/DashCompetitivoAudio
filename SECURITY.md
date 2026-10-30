# Política de Seguridad — DashCompetitivoAudio

Nos tomamos muy en serio la seguridad y estabilidad del software en Windows, dado que interactúa con endpoints de audio del sistema operativo y configuraciones locales.

---

## 1. Versiones Soportadas

| Versión | Soportada | Parches de Seguridad |
| :--- | :--- | :--- |
| **1.0.x** | **Sí** | Versión actual de soporte activo. |
| < 1.0.0 | No | Versiones preliminares obsoletas. |

---

## 2. Reporte de Vulnerabilidades
Si descubres una vulnerabilidad de seguridad en DashCompetitivoAudio, por favor **no abras una incidencia pública en GitHub**.

En su lugar, envía un reporte confidencial detallado a través del sistema de [GitHub Security Advisories](https://github.com/jose-seminario/DashCompetitivoAudio/security/advisories) o por correo a los administradores del repositorio.

Por favor incluye:
- Descripción del problema y severidad estimada (ej: escalación de privilegios, denegación de servicio por buffer overflow, inyección de archivo).
- Pasos de reproducción detallados o código de prueba (Proof of Concept).
- Sistema operativo y versión de Windows utilizada.

Nos comprometemos a acusar recibo en menos de 48 horas y a trabajar en una mitigación antes de cualquier divulgación pública.

---

## 3. Principios de Seguridad Implementados
- **Aislamiento de Perfiles**: Los perfiles son datos puros analizados por `ProfileValidator`. Se rechaza cualquier archivo con comandos ejecutables (`powershell`, `cmd.exe`, `.dll`, `.exe`).
- **Prevención de Path Traversal**: Todos los identificadores de perfil (`profileId`) deben ser slugs alfanuméricos simples sin caracteres `..`, `/` o `\`.
- **Escritura Atómica en Disco**: La inyección a Equalizer APO y el guardado de perfiles se realizan mediante archivos temporales con reemplazo atómico para evitar corrupción en caso de cierre abrupto.
- **Cero Telemetría Obligatoria**: No se transmiten datos acústicos, hábitos de juego ni información personal a servidores externos.

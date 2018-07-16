/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2016 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <string.h>

#include <retro_assert.h>
#include <compat/strl.h>
#include <string/stdstring.h>

#include "../msg_hash.h"
#include "../configuration.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Winvalid-source-encoding"
#endif

int menu_hash_get_help_es_enum(enum msg_hash_enums msg, char *s, size_t len)
{
   uint32_t driver_hash = 0;
   settings_t      *settings = config_get_ptr();

   switch (msg)
   {
      case MENU_ENUM_LABEL_CORE_LIST:
         snprintf(s, len,
               "Cargar núcleo. \n"
               " \n"
               "Busca una implementación de núcleo \n"
               "para libretro. El navegador empezará \n"
               "desde la ruta de tu carpeta de núcleos.\n"
               "Si está en blanco, empezará desde \n"
               "la raíz.\n"
               " \n"
               "Si la carpeta de núcleos es una carpeta,\n"
               "el menú la utilizará como carpeta \n"
               "base. Si la carpeta de núcleos es una \n"
               "ruta completa, empezará en la carpeta \n"
               "donde se encuentre el archivo.");
         break;
      case MENU_ENUM_LABEL_VALUE_MENU_ENUM_CONTROLS_PROLOG:
         snprintf(s, len,
               "Puedes usar estos controles en tu mando\n"
               "o teclado para controlar el menú: \n"
               " \n"
               );
         break;
      case MENU_ENUM_LABEL_WELCOME_TO_RETROARCH:
         snprintf(s, len,
               "Bienvenido a RetroArch\n"
               "\n"
               "Para más información ve al menú \n"
              "de Ayuda.\n"
               );
         break;
      case MENU_ENUM_LABEL_VALUE_HELP_AUDIO_VIDEO_TROUBLESHOOTING_DESC:
         {
            /* Work around C89 limitations */
            char u[501];

            const char * t =
                  "RetroArch utiliza un formato único para \n"
                  "sincronizar vídeo y sonido que necesita \n"
                  "calibrarse con la frecuencia de \n"
                  "actualización de tu monitor para obtener \n"
                  "el mejor rendimiento. \n"
                  " \n"
                  "Si notas cortes de sonido o en la imagen,\n"
                  "lo normal es que necesites calibrar estos\n"
                  "ajustes. Aquí van algunas opciones:\n"
                  " \n";
            snprintf(u, sizeof(u), /* can't inline this due to the printf arguments */
                  "a) Ve a '%s' -> '%s' y activa\n"
                  "'Vídeo por hilos'. En este modo la tasa\n"
                  "de refresco es irrelevante, habrá más fps,\n"
                  "'Vídeo multinúcleo'. En este modo la \n"
                  "frecuencia es irrelevante, habrá más fps,\n"
                  "pero la imagen podría ser menos fluida.\n"
                  "b) Ve a '%s' -> '%s' y busca\n"
                  "'%s'. Deja que se ejecute durante\n"
                  "2048 fotogramas y selecciona Aceptar.",
                  msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SETTINGS),
                  msg_hash_to_str(MENU_ENUM_LABEL_VALUE_VIDEO_SETTINGS),
                  msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SETTINGS),
                  msg_hash_to_str(MENU_ENUM_LABEL_VALUE_VIDEO_SETTINGS),
                  msg_hash_to_str(MENU_ENUM_LABEL_VALUE_VIDEO_REFRESH_RATE_AUTO)
                     );
            strlcpy(s, t, len);
            strlcat(s, u, len);
         }
         break;
      case MENU_ENUM_LABEL_VALUE_HELP_SCANNING_CONTENT_DESC:
         snprintf(s, len,
               "Para escanear contenidos ve a '%s' \n"
               "y selecciona '%s' o \n"
               "'%s'.\n"
               " \n"
               "Esto comparará los archivos con las entradas en \n"
               "la base de datos. Si hay una coincidencia, \n"
               "añadirá una entrada en una colección.\n"
               " \n"
               "Entonces podrás acceder fácilmente al contenido\n"
               "si vas a '%s' ->\n"
               "'%s'\n"
               "en vez de tener que pasar por el navegador \n"
               "de archivos constantemente.\n"
               " \n"
               "NOTA: El contenido de algunos núcleos podría\n"
               "no ser localizable. Entre los ejemplos están\n"
               "PlayStation, MAME, FBA, y puede que otros."
               ,
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_ADD_CONTENT_LIST),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SCAN_DIRECTORY),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SCAN_FILE),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_LIST),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_CONTENT_COLLECTION_LIST)
               );
         break;
      case MENU_ENUM_LABEL_VALUE_EXTRACTING_PLEASE_WAIT:
         strlcpy(s, "Extrayendo, espera, por favor...\n", len);
         break;
      case MENU_ENUM_LABEL_INPUT_DRIVER:
         if (settings)
            driver_hash = msg_hash_calculate(settings->input.driver);

         switch (driver_hash)
         {
            case MENU_LABEL_INPUT_DRIVER_UDEV:
               {
                  /* Work around C89 limitations */
                  char u[501];
                  char t[501];

                  strlcpy(t,
                        "Controlador de entrada udev. \n"
                        " \n"
                        "Este controlador puede funcionar sin X. \n"
                        " \n"
                        "Utiliza la API más reciente para joypads \n"
                        "evdec para dar compatibilidad con joysticks. \n"
                        "Permite conexión en caliente y force \n"
                        "feedback (si lo admite el dispositivo). \n",
                        sizeof(t));
                  strlcpy(u,
                        " \n"
                        "El controlador lee los eventos evdev para \n"
                        "dar compatibilidad con teclados. También \n"
                        "es compatible con retrollamadas de teclado, \n"
                        "ratones y pantallas táctiles. \n"
                        " \n"
                        "La mayoría de las distros tienen los nodos \n"
                        "/dev/input en modo root-only (modo 600). \n"
                        "Puedes configurar una regla udev que los haga \n"
                        "accesibles fuera de la raíz.", sizeof(u)
                        );

                  strlcpy(s, t, len);
                  strlcat(s, u, len);
               }
               break;
            case MENU_LABEL_INPUT_DRIVER_LINUXRAW:
               snprintf(s, len,
                     "Controlador de entrada linuxraw. \n"
                     " \n"
                     "Este controlador necesita de un TTY activo. \n"
                     "Los eventos de teclado se leen directamente \n"
                     "desde el TTY, lo que es más simple pero no tan \n"
                     "flexible como udev. \n"
                     "No es compatible con ratones, etc. \n"
                     " \n"
                     "Este controlador utiliza la antigua API de \n"
                     "joysticks (/dev/input/js*).");
               break;
            default:
               snprintf(s, len,
                     "Controlador de entrada.\n"
                     " \n"
                     "El controlador de vídeo podría forzar \n"
                     "el uso de un controlador de entrada \n"
                     "distinto.");
               break;
         }
         break;
      case MENU_ENUM_LABEL_LOAD_CONTENT_LIST:
         snprintf(s, len,
               "Cargar contenido. \n"
               "Buscar contenido. \n"
               " \n"
               "Para cargar contenidos necesitas \n"
               "un 'núcleo' y un archivo de contenido.\n"
               " \n"
               "Para controlar el lugar donde el menú \n"
               "empieza a buscar contenidos, cambia \n"
               "la opción 'Carpeta del navegador de \n"
               "archivos'. En caso de que no esté \n"
               "configurada, empezará desde la raíz.\n"
               " \n"
               "El navegador filtrará las extensiones \n"
               "del último núcleo seleccionado en \n"
               "'Cargar núcleo' y lo utilizará al \n"
               "cargar un contenido."
               );
         break;
      case MENU_ENUM_LABEL_LOAD_CONTENT_HISTORY:
         snprintf(s, len,
               "Cargar contenido del historial. \n"
               " \n"
               "Cuando se cargan contenidos, estos y \n"
               "las combinaciones de núcleos de libretro \n"
               "se guardan en el historial. \n"
               " \n"
               "El historial se guarda en un archivo en la \n"
               "misma carpeta que el archivo de configura- \n"
               "ción de RetroArch. Si no se ha cargado un \n"
               "archivo de configuración al iniciar, no se \n"
               "guardará ni cargará el historial, y la \n"
               "opción no existirá en el menú principal."
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_DRIVER:
         snprintf(s, len,
               "Controlador de vídeo actual.");

         if (string_is_equal(settings->video.driver, "gl"))
         {
            snprintf(s, len,
                  "Controlador de vídeo OpenGL. \n"
                  " \n"
                  "Este controlador permite que los núcleos \n"
                  "libretro GL se utilicen, además de las \n"
                  "implementaciones renderizadas por\n"
                  "software del núcleo.\n"
                  " \n"
                  "El rendimiento de las implementaciones \n"
                  "por software y libretro GL dependen \n"
                  "del controlador GL que tenga tu \n"
                  "tarjeta gráfica.");
         }
         else if (string_is_equal(settings->video.driver, "sdl2"))
         {
            snprintf(s, len,
                  "Controlador de vídeo SDL 2.\n"
                  " \n"
                  "Este es un controlador de vídeo por \n"
                  "software SDL 2.\n"
                  " \n"
                  "El rendimiento para las implementaciones \n"
                  "libretro por software depende de la \n"
                  "implementación SDL de tu plataforma.");
         }
         else if (string_is_equal(settings->video.driver, "sdl1"))
         {
            snprintf(s, len,
                  "Controlador de vídeo SDL.\n"
                  " \n"
                  "Este es un controlador de vídeo por \n"
                  "software SDL 1.2.\n"
                  " \n"
                  "Su rendimiento es considerado inferior \n"
                  "a lo óptimo. Utilízalo únicamente como \n"
                  "último recurso.");
         }
         else if (string_is_equal(settings->video.driver, "d3d"))
         {
            snprintf(s, len,
                  "Controlador de vídeo Direct3D. \n"
                  " \n"
                  "El rendimiento de los núcleos que \n"
                  "rendericen por software dependerá \n"
                  "del controlador D3D de tu tarjeta \n"
                  "gráfica.");
         }
         else if (string_is_equal(settings->video.driver, "exynos"))
         {
            snprintf(s, len,
                  "Controlador de vídeo Exynos-G2D. \n"
                  " \n"
                  "Este es un controlador de vídeo Exynos \n"
                  "de bajo nivel. Utiliza el bloque G2D \n"
                  "del SoC Exynos de Samsung para las \n"
                  "operaciones de blit. \n"
                  " \n"
                  "El rendimiento de los núcleos \n"
                  "renderizados por software debería \n"
                  "ser óptimo.");
         }
         else if (string_is_equal(settings->video.driver, "drm"))
         {
            snprintf(s, len,
                  "Controlador de vídeo de DRM simple. \n"
                  " \n"
                  "Este es un controlador de vídeo que \n"
                  "usa libdrm para escalado por hardware \n"
                  "mediante los overlays de la GPU. \n"
                  " \n"
                  "El blitting se hace por software.");
         }
         else if (string_is_equal(settings->video.driver, "sunxi"))
         {
            snprintf(s, len,
                  "Controlador de vídeo Sunxi-G2D. \n"
                  " \n"
                  "Este es un controlador de vídeo Sunxi \n"
                  "de bajo nivel. Utiliza el bloque G2D \n"
                  "de todos los SoC Allwinner.");
         }
         break;
      case MENU_ENUM_LABEL_AUDIO_DSP_PLUGIN:
         snprintf(s, len,
               "Plugin de sonido DSP.\n"
               "Procesa el sonido antes de enviarlo \n"
               "al controlador."
               );
         break;
      case MENU_ENUM_LABEL_AUDIO_RESAMPLER_DRIVER:
         if (settings)
            driver_hash = msg_hash_calculate(settings->audio.resampler);

         switch (driver_hash)
         {
            case MENU_LABEL_AUDIO_RESAMPLER_DRIVER_SINC:
               snprintf(s, len,
                     "Implementación SINC en ventana.");
               break;
            case MENU_LABEL_AUDIO_RESAMPLER_DRIVER_CC:
               snprintf(s, len,
                     "Implementación de cosenos complejos.");
               break;
         }
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET:
         snprintf(s, len,
               "Cargar preajustes de shaders. \n"
               " \n"
               " Carga directamente un preajuste "
#ifdef HAVE_CG
               "Cg"
#endif
#ifdef HAVE_GLSL
#ifdef HAVE_CG
               "/"
#endif
               "GLSL"
#endif
#ifdef HAVE_HLSL
#if defined(HAVE_CG) || defined(HAVE_HLSL)
               "/"
#endif
               "HLSL"
#endif
               ". \n"
               "El menú de shaders se actualizará. \n"
               " \n"
               "Si el CGP utiliza métodos de escalado \n"
               "complejos (por ejemplo, escalado de \n"
               "origen, el mismo factor para X/Y), podría \n"
               "no mostrar un factor de escalado correcto \n"
               "en el menú."
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_SCALE_PASS:
         snprintf(s, len,
               "La escala de esta pasada. \n"
               " \n"
               "El factor de escala es acumulativo, \n"
               "por ejemplo: 2x para el primer pase y \n"
               "2x para el segundo dará una escala \n"
               "total de 4x. \n"
               " \n"
               "Si el último pase tiene un factor de \n"
               "escala, el resultado se estirará por \n"
               "toda la pantalla con el filtro espe- \n"
               "cificado en 'Filtro predeterminado'. \n"
               " \n"
               "Si has seleccionado 'No importa', se \n"
               "utilizará o bien la escala 1x o se \n"
               "estirará a pantalla completa en función \n"
               "de si es el último pase o no."
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_NUM_PASSES:
         snprintf(s, len,
               "Pases de shaders. \n"
               " \n"
               "RetroArch permite mezclar diversos shaders \n"
               "con pasadas arbitrarias, filtros persona- \n"
               "lizados de hardware y factores de escala. \n"
               " \n"
               "Esta opción especifica la cantidad de pasadas \n"
               "de shaders a utilizar. Si seleccionas 0 y \n"
               "luego 'Aplicar cambios en shaders', \n"
               "utilizarás un shader 'en blanco'. \n"
               " \n"
               "La opción filtro predeterminado afectará \n"
               "al filtro de estiramiento.");
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_PARAMETERS:
         snprintf(s, len,
               "Parámetros de shaders. \n"
               " \n"
               "Modifica directamente el shader actual. \n"
               "No se guardará en el preajuste CGP/GLSLP.");
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_PRESET_PARAMETERS:
         snprintf(s, len,
               "Parámetros del preajuste de shaders. \n"
               " \n"
               "Modifica el preajuste de shaders que \n"
               "se encuentra actualmente en el menú."
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_PASS:
         snprintf(s, len,
               "Ruta al shader. \n"
               " \n"
               "Todos los shaders deben ser del \n"
               "mismo tipo (por ejemplo: CG, GLSL \n"
               "o HLSL). \n"
               " \n"
               "Modifica la carpeta de shaders para \n"
               "indicar la ruta en la que el navegador \n"
               "comenzará a buscar shaders."
               );
         break;
      case MENU_ENUM_LABEL_CONFIG_SAVE_ON_EXIT:
         snprintf(s, len,
               "Guarda la configuración al salir.\n"
               "Es útil para el menú, ya que los \n"
               "ajustes pueden ser modificados. \n"
               "Sobrescribirá la configuración. \n"
               " \n"
               "No se preservarán los #include \n"
               "y los comentarios. \n"
               " \n"
               "El archivo de configuración se \n"
               "considera inmutable por diseño \n"
               "ya que es probable que el usuario \n"
               "lo controle, y no debería ser \n"
               "sobrescrito sin que el usuario \n"
               "no se entere."
#if defined(RARCH_CONSOLE) || defined(RARCH_MOBILE)
               "\nSin embargo, no suele ser el caso \n"
               "en consolas, donde no es posible \n"
               "examinar el archivo de configuración \n"
               "de forma manual."
#endif
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_SHADER_FILTER_PASS:
         snprintf(s, len,
               "Filtro de hardware para esta pasada. \n"
               " \n"
               "Si se ha seleccionado 'No importa', \n"
               "se utilizará el filtro predeterminado."
               );
         break;
      case MENU_ENUM_LABEL_AUTOSAVE_INTERVAL:
         snprintf(s, len,
               "Guarda automáticamente la SRAM \n"
               "no volátil en intervalos regulares.\n"
               " \n"
               "Esta opción está desactivada por \n"
               "defecto a menos que se haya configurado \n"
               "lo contrario. El intervalo se mide \n"
               "en segundos. \n"
               " \n"
               "Si utlizas 0, desactivarás el \n"
               "guardado automático.");
         break;
      case MENU_ENUM_LABEL_INPUT_BIND_DEVICE_TYPE:
         snprintf(s, len,
               "Tipo de dispositivo de entrada. \n"
               " \n"
               "Selecciona el tipo de dispositivo a \n"
               "utilizar. Es relevante para el núcleo \n"
               "de libretro."
               );
         break;
      case MENU_ENUM_LABEL_LIBRETRO_LOG_LEVEL:
         snprintf(s, len,
               "Ajusta el nivel de registro de los \n"
               "núcleos de libretro (GET_LOG_INTERFACE). \n"
               " \n"
               " Si un nivel de registro indicado por \n"
               " un núcleo de libretro está por debajo \n"
               " del nivel indicado en libretro_log, \n"
               " será ignorado.\n"
               " \n"
               " Los registros DEBUG siempre son ignorados \n"
               " a menos que esté activado el modo \n"
               " de verbosidad (--verbose).\n"
               " \n"
               " DEBUG = 0\n"
               " INFO  = 1\n"
               " WARN  = 2\n"
               " ERROR = 3"
               );
         break;
      case MENU_ENUM_LABEL_STATE_SLOT_INCREASE:
      case MENU_ENUM_LABEL_STATE_SLOT_DECREASE:
         snprintf(s, len,
               "Ranuras de guardados rápidos.\n"
               " \n"
               " Si se selecciona la ranura 0, el nombre \n"
               " del guardado rápido es *.state \n"
               " (o lo que esté definido en la línea \n"
               "de comandos).\n"
               "Si la ranura es != 0, la ruta será (path)(d), \n"
               "siendo (d) el número de ranura.");
         break;
      case MENU_ENUM_LABEL_SHADER_APPLY_CHANGES:
         snprintf(s, len,
               "Aplicar cambios en shaders. \n"
               " \n"
               "Después de cambiar los ajustes de shaders, \n"
               "utiliza esta opción para aplicar los \n"
               "cambios. \n"
               " \n"
               "Los cambios en los shaders es una \n"
               "operación que ocupa bastante memoria, \n"
               "así que han de hacerse de forma \n"
               "explícita. \n"
               " \n"
               "Al aplicar shaders, se guarda su configuración \n"
               "a un archivo temporal (menu.cgp ormenu.glslp) \n"
               "y se cargan. El archivo persistirá al \n"
               "abandonar RetroArch. El archivo se guarda en \n"
               "la carpeta Shader."
               );
         break;
      case MENU_ENUM_LABEL_MENU_TOGGLE:
         snprintf(s, len,
               "Muestra u oculta el menú.");
         break;
      case MENU_ENUM_LABEL_GRAB_MOUSE_TOGGLE:
         snprintf(s, len,
               "Permite o no capturar el ratón.\n"
               " \n"
               "Al capturar el ratón, RetroArch lo oculta \n"
               "y mantiene el puntero del ratón dentro de \n"
               "la ventana para que la entrada relativa \n"
               "del ratón funcione mejor.");
         break;
      case MENU_ENUM_LABEL_DISK_NEXT:
         snprintf(s, len,
               "Rota entre las imágenes de disco. \n"
               "Utiliza esta opción después de \n"
               "expulsar un disco. \n"
               " \n"
               " Termina la operación volviendo \n"
               " a conmutar el botón de expulsión.");
         break;
      case MENU_ENUM_LABEL_VIDEO_FILTER:
#ifdef HAVE_FILTERS_BUILTIN
         snprintf(s, len,
               "Filtro de vídeo basado en la CPU.");
#else
         snprintf(s, len,
               "Filtro de vídeo basado en la CPU.\n"
               " \n"
               "Ruta a una librería dinámica.");
#endif
         break;
      case MENU_ENUM_LABEL_AUDIO_DEVICE:
         snprintf(s, len,
               "Anula el dispositivo de sonido \n"
               "predeterminado que utiliza el \n"
               "controlador de sonido.\n"
               "Esta opción depende del contro- \n"
               "lador. Por ejemplo:\n"
#ifdef HAVE_ALSA
               " \n"
               "ALSA necesita un dispositivo PCM."
#endif
#ifdef HAVE_OSS
               " \n"
               "OSS necesita una ruta (por ejemplo: \n"
               "/dev/dsp)."
#endif
#ifdef HAVE_JACK
               " \n"
               "JACK necesita nombres de puertos \n"
               "(por ejemplo: system:playback1\n"
               ",system:playback_2)."
#endif
#ifdef HAVE_RSOUND
               " \n"
               "RSound necesita una dirección IP \n"
               "de un servidor RSound."
#endif
               );
         break;
      case MENU_ENUM_LABEL_DISK_EJECT_TOGGLE:
         snprintf(s, len,
               "Alterna el botón de expulsión \n"
               "de discos.\n"
               " \n"
               "Se utiliza para contenidos \n"
               "que utilicen varios discos.");
         break;
      case MENU_ENUM_LABEL_ENABLE_HOTKEY:
         snprintf(s, len,
               "Activa otras teclas rápidas.\n"
               " \n"
               "Si esta tecla rápida está asignada a un \n"
               "teclado, un botón o un eje de un joystick, \n"
               "el resto de teclas rápidas se desactivarán \n"
               "a menos que esta tecla se mantenga pulsada \n"
               "al mismo tiempo. \n"
               " \n"
               "Esto sirve para implementaciones centradas \n"
               "en RETRO_KEYBOARD, que ocupan una gran \n"
               "porción del teclado y no es una buena idea \n"
               "que las teclas rápidas interfieran con \n"
               "el funcionamiento normal.");
         break;
      case MENU_ENUM_LABEL_REWIND_ENABLE:
         snprintf(s, len,
               "Activa el rebobinado.\n"
               " \n"
               "Esto consume rendimiento, así que \n"
               "está desactivado por defecto.");
         break;
      case MENU_ENUM_LABEL_LIBRETRO_DIR_PATH:
         snprintf(s, len,
               "Carpeta de núcleos. \n"
               " \n"
               "Una carpeta donde se buscarán las \n"
               "implementaciones de núcleos libretro.");
         break;
      case MENU_ENUM_LABEL_VIDEO_REFRESH_RATE_AUTO:
         {
            /* Work around C89 limitations */
            char u[501];
            char t[501];

            strlcpy(t,
                  "Frecuencia de actualización automática.\n"
                  " \n"
                  "La frecuencia de actualización precisa del \n"
                  "monitor (en Hz). Se utiliza para calcular \n"
                  "la frecuencia de entrada de sonido con esta \n"
                  "fórmula: \n"
                  " \n"
                  "audio_input_rate = veloc. de entrada de juego \n"
                  "* frecuencia de actualización de pantalla / \n"
                  "frecuencia de actualización de juego \n"
                  " \n", sizeof(t));
            strlcpy(u,
                  "Si la implementación no indica un valor, se \n"
                  "asumirá de forma predeterminada el sistema \n"
                  "NTSC por compatibilidad.\n"
                  " \n"
                  "Este valor debería ser lo más similar a 60Hz \n"
                  "para evitar cambios en el tono. Si tu \n"
                  "monitor no funciona a 60Hz o similar, \n"
                  "desactiva la sincronía vertical y deja \n"
                  "esta opción en su valor predeterminado.", sizeof(u));
            strlcpy(s, t, len);
            strlcat(s, u, len);
         }
         break;
      case MENU_ENUM_LABEL_VIDEO_ROTATION:
         snprintf(s, len,
               "Fuerza una rotación concreta \n"
               "de la pantalla.\n"
               " \n"
               "La rotación se añade a las rotaciones \n"
               "que indique el núcleo libretro (ver \n"
               "Permitir rotación de vídeo).");
         break;
      case MENU_ENUM_LABEL_VIDEO_SCALE:
         snprintf(s, len,
               "Resolución en pantalla completa.\n"
               " \n"
               "Si se indica 0, se utilizará la \n"
               "resolución del entorno.\n");
         break;
      case MENU_ENUM_LABEL_FASTFORWARD_RATIO:
         snprintf(s, len,
               "Proporción de aceleración."
               " \n"
               "Indica la velocidad máxima a la que \n"
               "se ejecutará el contenido mientras \n"
               "es acelerado.\n"
               " \n"
               " (Por ejemplo, 5.0 para un contenido \n"
               "a 60 FPS => Máximo de 300 FPS).\n"
               " \n"
               "RetroArch pasará a segundo plano \n"
               "para asegurarse de que no se supere \n"
               "la velocidad máxima. Este límite \n"
               "no es completamente preciso.");
         break;
      case MENU_ENUM_LABEL_VIDEO_MONITOR_INDEX:
         snprintf(s, len,
               "Monitor preferido.\n"
               " \n"
               "0 (predeterminado) significa que no hay \n"
               "preferencia por un monitor en concreto, \n"
               "y de 1 en adelante (siendo 1 el primer \n"
               "monitor) sugiere a RetroArch que \n"
               "utilice ese monitor específico.");
         break;
      case MENU_ENUM_LABEL_VIDEO_CROP_OVERSCAN:
         snprintf(s, len,
               "Obliga a recortar los fotogramas \n"
               "con overscan.\n"
               " \n"
               "El comportamiento exacto de esta \n"
               "opción depende de la implementación \n"
               "del núcleo.");
         break;
      case MENU_ENUM_LABEL_VIDEO_SCALE_INTEGER:
         snprintf(s, len,
               "Solo escala el vídeo con números \n"
               "enteros.\n"
               " \n"
               "El tamaño base depende de la geometría \n"
               "que indique el sistema y de la \n"
               "proporción de aspecto.\n"
               " \n"
               "Si no se ha configurado la opción \n"
               "Forzar proporción, los valores X/Y \n"
               "se escalarán de forma independiente \n"
               "según números enteros.");
         break;
      case MENU_ENUM_LABEL_AUDIO_VOLUME:
         snprintf(s, len,
               "Volumen de sonido expresado en dB.\n"
               " \n"
               "0 dB es el volumen normal, sin ganancia \n"
               "aplicada. La ganancia se puede controlar \n"
               "en tiempo real con los botones de \n"
               "Subir volumen / Bajar volumen.");
         break;
      case MENU_ENUM_LABEL_AUDIO_RATE_CONTROL_DELTA:
         snprintf(s, len,
               "Control de la frecuencia de sonido.\n"
               " \n"
               "Si seleccionas 0, desactivarás el control \n"
               "de la frecuencia. Cualquier otro valor \n"
               "cambiará el delta de control de la \n"
               "frecuencia de sonido.\n"
               " \n"
               "Define cuánta frecuencia de entrada puede \n"
               "ajustarse de forma dinámica.\n"
               " \n"
               " La frecuencia de entrada se define como: \n"
               " frecuencia de entrada * (1.0 +/- \n"
               "(delta de control de frecuencia))");
         break;
      case MENU_ENUM_LABEL_AUDIO_MAX_TIMING_SKEW:
         snprintf(s, len,
               "Variación máxima en la sincronía de \n"
               "sonido.\n"
               " \n"
               "Define la variación máxima de la \n"
               "frecuencia de entrada. Podrías aumentar \n"
               "el valor para cambiar la sincronía, por \n"
               "ejemplo, si ejecutas núcleos PAL en \n"
               "monitores NTSC, a cambio de tener un \n"
               "tono de sonido impreciso.\n"
               " \n"
               " La frecuencia de entrada se define como: \n"
               " frecuencia de entrada * (1.0 +/- \n"
               "(variación máxima de sincronía))");
         break;
      case MENU_ENUM_LABEL_OVERLAY_NEXT:
         snprintf(s, len,
               "Cambia a la siguiente superposición.\n"
               " \n"
               "Se expande alrededor.");
         break;
      case MENU_ENUM_LABEL_LOG_VERBOSITY:
         snprintf(s, len,
               "Activa o desactiva el nivel de \n"
               "verbosidad del frontend.");
         break;
      case MENU_ENUM_LABEL_VOLUME_UP:
         snprintf(s, len,
               "Aumenta el volumen del sonido.");
         break;
      case MENU_ENUM_LABEL_VOLUME_DOWN:
         snprintf(s, len,
               "Disminuye el volumen del sonido.");
         break;
      case MENU_ENUM_LABEL_VIDEO_DISABLE_COMPOSITION:
         snprintf(s, len,
               "Desactiva por la fuerza la composición.\n"
               "Actualmente solo funciona en \n"
               "Windows Vista/7.");
         break;
      case MENU_ENUM_LABEL_PERFCNT_ENABLE:
         snprintf(s, len,
               "Activa o desactiva los contadores \n"
               "de rendimiento del frontend.");
         break;
      case MENU_ENUM_LABEL_SYSTEM_DIRECTORY:
         snprintf(s, len,
               "Carpeta de sistemas. \n"
               " \n"
               "Asigna la carpeta 'system'.\n"
               "Los núcleos pueden buscar dentro \n"
               "de ella para cargar BIOS, \n"
               "configuraciones específicas para \n"
               "un sistema, etc.");
         break;
      case MENU_ENUM_LABEL_SAVESTATE_AUTO_SAVE:
         snprintf(s, len,
               "Guarda automáticamente un guardado rápido \n"
               "al terminar la ejecución de RetroArch.\n"
               " \n"
               "RetroArch cargará automáticamente cualquier \n"
               "guardado rápido que se encuentre en esa ruta \n"
               "al arrancar si la opción 'Cargar guardado \n"
               "rápido automáticamente' está activada.");
         break;
      case MENU_ENUM_LABEL_VIDEO_THREADED:
         snprintf(s, len,
               "Utilizar un controlador de vídeo \n"
               "por hilos.\n"
               " \n"
               "Esta opción podría mejorar el rendimiento \n"
               "a costa de ganar retraso y perder fluidez \n"
               "de vídeo.");
         break;
      case MENU_ENUM_LABEL_VIDEO_VSYNC:
         snprintf(s, len,
               "Sincronía vertical para vídeo.\n");
         break;
      case MENU_ENUM_LABEL_VIDEO_HARD_SYNC:
         snprintf(s, len,
               "Intenta sincronizar la CPU y la GPU \n"
               "de una forma más estricta.\n"
               " \n"
               "Puede reducir el retraso a costa \n"
               "de perder rendimiento.");
         break;
      case MENU_ENUM_LABEL_REWIND_GRANULARITY:
         snprintf(s, len,
               "Nivel de detalle del rebobinado.\n"
               " \n"
               "Cuando se rebobina un número determinado \n"
               "de fotogramas, puedes rebobinar varios \n"
               "fotogramas a la vez, aumentando la \n"
               "velocidad de rebobinado.");
         break;
      case MENU_ENUM_LABEL_SCREENSHOT:
         snprintf(s, len,
               "Capturar la pantalla.");
         break;
      case MENU_ENUM_LABEL_VIDEO_FRAME_DELAY:
         snprintf(s, len,
               "Ajusta la cantidad de retraso en \n"
               "milisegundos después de la sincronía \n"
               "vertical antes de ejecutar el núcleo.\n"
               "\n"
               "Puede reducir el retraso a costa de \n"
               "aumentar la probabilidad de perder \n"
               "fluidez.\n"
               " \n"
               "El valor máximo es 15.");
         break;
      case MENU_ENUM_LABEL_VIDEO_HARD_SYNC_FRAMES:
         snprintf(s, len,
               "Ajusta la cantidad de fotogramas \n"
               "que puede adelantar la CPU a la GPU \n"
               "al utilizar 'Sincronía estricta \n"
               "de CPU'.\n"
               " \n"
               "El valor máximo es 3.\n"
               " \n"
               " 0: Sincroniza inmediatamente a GPU.\n"
               " 1: Sincroniza con el fotograma anterior.\n"
               " 2: Etc...");
         break;
      case MENU_ENUM_LABEL_VIDEO_BLACK_FRAME_INSERTION:
         snprintf(s, len,
               "Introduce un fotograma negro \n"
               "entre fotogramas.\n"
               " \n"
               "Es útil para los monitores que \n"
               "funcionan a 120 Hz y quieren jugar \n"
               "a material a 60 Hz eliminando el \n"
               "ghosting.\n"
               " \n"
               "La frecuencia de actualización de vídeo \n"
               "aún debe configurarse como si utilizaras \n"
               "un monitor de 60 Hz (divide la frecuencia \n"
               "de actualización entre 2).");
         break;
      case MENU_ENUM_LABEL_RGUI_SHOW_START_SCREEN:
         snprintf(s, len,
               "Muestra la pantalla de inicio en el menú.\n"
               "Se desactiva automáticamente cuando se \n"
               "ha visto por primera vez.\n"
               " \n"
               "Esta opción solo se actualiza dentro \n"
               "de la configuración si 'Guardar \n"
               "configuración al salir' está activado.\n");
         break;
      case MENU_ENUM_LABEL_CORE_SPECIFIC_CONFIG:
         snprintf(s, len,
               "Carga un archivo de configuración \n"
               "concreto en función del núcleo \n"
               "que se vaya a utilizar.\n");
         break;
      case MENU_ENUM_LABEL_VIDEO_FULLSCREEN:
         snprintf(s, len,
                "Activa o desactiva el modo \n"
                "en pantalla completa.");
         break;
      case MENU_ENUM_LABEL_BLOCK_SRAM_OVERWRITE:
         snprintf(s, len,
               "Impide que la SRAM sea sobrescrita \n"
               "mientras se carga un guardado rápido.\n"
               " \n"
               "Podría provocar defectos en los \n"
               "juegos.");
         break;
      case MENU_ENUM_LABEL_PAUSE_NONACTIVE:
         snprintf(s, len,
               "Pausa el juego cuando la ventana \n"
               "pasa a estar en segundo plano.");
         break;
      case MENU_ENUM_LABEL_VIDEO_GPU_SCREENSHOT:
         snprintf(s, len,
               "Si es posible, las capturas de \n"
               "pantalla se realizan del material \n"
               "pos-shaders de GPU.");
         break;
      case MENU_ENUM_LABEL_SCREENSHOT_DIRECTORY:
         snprintf(s, len,
               "Carpeta de capturas de pantalla. \n"
               " \n"
               "En esta carpeta se guardarán \n"
               "las capturas de pantalla."
               );
         break;
      case MENU_ENUM_LABEL_VIDEO_SWAP_INTERVAL:
         snprintf(s, len,
               "Intervalo de cambio de la \n"
               "sincronía vertical.\n"
               " \n"
               "Utiliza un intervalo personalizado para \n"
               "la sincronía vertical. Utiliza este \n"
               "parámetro para reducir a la mitad \n"
               "la frecuencia de actualización \n"
               "del monitor.");
         break;
      case MENU_ENUM_LABEL_SAVEFILE_DIRECTORY:
         snprintf(s, len,
               "Carpeta de partidas guardadas. \n"
               " \n"
               "Almacena todas las partidas guardadas \n"
               "(*.srm) en esta carpeta. Incluye los \n"
               "archivos relacionados, como .bsv, .rt, \n"
               ".psrm, etcétera.\n"
               " \n"
               "Esta carpeta será ignorada si se \n"
               "utilizan explícitamente las opciones por \n"
               "línea de comandos.");
         break;
      case MENU_ENUM_LABEL_SAVESTATE_DIRECTORY:
         snprintf(s, len,
               "Carpeta de guardados rápidos. \n"
               " \n"
               "Almacena todos los guardados rápidos \n"
               "(*.state) en esta carpeta.\n"
               " \n"
               "Esta carpeta será ignorada si se \n"
               "utilizan explícitamente las opciones por \n"
               "línea de comandos.");
         break;
      case MENU_ENUM_LABEL_ASSETS_DIRECTORY:
         snprintf(s, len,
               "Carpeta de recursos. \n"
               " \n"
               "Esta ubicación es donde las interfaces \n"
               "de menús buscan de forma predeterminada \n"
               "los recursos a cargar, etc.");
         break;
      case MENU_ENUM_LABEL_DYNAMIC_WALLPAPERS_DIRECTORY:
         snprintf(s, len,
               "Carpeta de fondos dinámicos de pantalla. \n"
               " \n"
               "Aquí se almacenan los fondos de pantalla \n"
               "que el menú cargará de forma dinámica \n"
               "según el contexto.");
         break;
      case MENU_ENUM_LABEL_SLOWMOTION_RATIO:
         snprintf(s, len,
               "Cantidad de velocidad reducida."
               " \n"
               "Al reducir la velocidad, el contenido \n"
               "se ralentizará según este factor.");
         break;
      case MENU_ENUM_LABEL_INPUT_AXIS_THRESHOLD:
         snprintf(s, len,
               "Define el margen de los ejes.\n"
               " \n"
               "Indica la distancia mínima que debe \n"
               "recorrer un eje para que provoque \n"
               "una pulsación del botón.\n"
               "Los valores posibles son [0.0, 1.0].");
         break;
      case MENU_ENUM_LABEL_INPUT_TURBO_PERIOD:
         snprintf(s, len,
               "Período de turbo.\n"
               " \n"
               "Describe la velocidad con la que se \n"
               "pulsan los botones en el modo turbo."
               );
         break;
      case MENU_ENUM_LABEL_INPUT_AUTODETECT_ENABLE:
         snprintf(s, len,
               "Activa la autoconfiguración de \n"
               "entrada. \n"
               " \n"
               "Intentará configurar de forma \n"
               "automática los mandos que se \n"
               "conecten en cualquier momento, \n"
               "como el sistema Plug-and-play.");
         break;
      case MENU_ENUM_LABEL_CAMERA_ALLOW:
         snprintf(s, len,
               "Permite o no el acceso a la cámara \n"
               "por parte de los núcleos.");
         break;
      case MENU_ENUM_LABEL_LOCATION_ALLOW:
         snprintf(s, len,
               "Permite o no los servicios de \n"
               "localización por parte de los núcleos.");
         break;
      case MENU_ENUM_LABEL_TURBO:
         snprintf(s, len,
               "Permite usar el turbo.\n"
               " \n"
               "Si mantienes pulsado el botón de turbo \n"
               "mientras pulsas otro botón, este último \n"
               "entrará en un modo turbo en el que el \n"
               "estado del botón se modula mediante una \n"
               "señal periódica. \n"
               " \n"
               "La modulación se detiene cuando se deja \n"
               "de pulsar el botón original (no el botón \n"
               "de turbo).");
         break;
      case MENU_ENUM_LABEL_OSK_ENABLE:
         snprintf(s, len,
               "Activa o desactiva el teclado \n"
               "en pantalla.");
         break;
      case MENU_ENUM_LABEL_AUDIO_MUTE:
         snprintf(s, len,
               "Silencia o no el sonido.");
         break;
      case MENU_ENUM_LABEL_REWIND:
         snprintf(s, len,
               "Mantén pulsado este botón para rebobinar.\n"
               " \n"
               "Para que este botón funcione, debes tener \n"
               "activada la opción de rebobinar.");
         break;
      case MENU_ENUM_LABEL_EXIT_EMULATOR:
         snprintf(s, len,
               "Asigna una tecla para abandonar \n"
               "RetroArch limpiamente."
#if !defined(RARCH_MOBILE) && !defined(RARCH_CONSOLE)
               "\nSi cierras el programa con \n"
               "cualquier forma brusca (SIGKILL, \n"
               "etc.) no se guardará la RAM, etc. \n"
               "En sistemas Unix, SIGINT/SIGTERM \n"
               "permite un cierre limpio."
#endif
               );
         break;
      case MENU_ENUM_LABEL_LOAD_STATE:
         snprintf(s, len,
               "Carga el guardado rápido.");
         break;
      case MENU_ENUM_LABEL_SAVE_STATE:
         snprintf(s, len,
               "Guarda rápidamente la partida.");
         break;
      case MENU_ENUM_LABEL_NETPLAY_FLIP_PLAYERS:
         snprintf(s, len,
               "Cambia los usuarios en red.");
         break;
      case MENU_ENUM_LABEL_CHEAT_INDEX_PLUS:
         snprintf(s, len,
               "Aumenta el índice de trucos.\n");
         break;
      case MENU_ENUM_LABEL_CHEAT_INDEX_MINUS:
         snprintf(s, len,
               "Disminuye el índice de trucos.\n");
         break;
      case MENU_ENUM_LABEL_SHADER_PREV:
         snprintf(s, len,
               "Aplica el shader anterior de la carpeta.");
         break;
      case MENU_ENUM_LABEL_SHADER_NEXT:
         snprintf(s, len,
               "Aplica el siguiente shader de la carpeta.");
         break;
      case MENU_ENUM_LABEL_RESET:
         snprintf(s, len,
               "Reinicia el contenido.\n");
         break;
      case MENU_ENUM_LABEL_PAUSE_TOGGLE:
         snprintf(s, len,
               "Alterna el estado en pausa.");
         break;
      case MENU_ENUM_LABEL_CHEAT_TOGGLE:
         snprintf(s, len,
               "Alterna el índice de trucos.\n");
         break;
      case MENU_ENUM_LABEL_HOLD_FAST_FORWARD:
         snprintf(s, len,
               "Mantén pulsado este botón para avanzar \n"
               "rápidamente. Suéltalo para desactivar \n"
               "esta función.");
         break;
      case MENU_ENUM_LABEL_SLOWMOTION:
         snprintf(s, len,
               "Mantén pulsado este botón para \n"
               "ir a cámara lenta.");
         break;
      case MENU_ENUM_LABEL_FRAME_ADVANCE:
         snprintf(s, len,
               "Avanza un fotograma mientras \n"
               "el contenido esté en pausa.");
         break;
      case MENU_ENUM_LABEL_MOVIE_RECORD_TOGGLE:
         snprintf(s, len,
               "Activa o desactiva la grabación.");
         break;
      case MENU_ENUM_LABEL_L_X_PLUS:
      case MENU_ENUM_LABEL_L_X_MINUS:
      case MENU_ENUM_LABEL_L_Y_PLUS:
      case MENU_ENUM_LABEL_L_Y_MINUS:
      case MENU_ENUM_LABEL_R_X_PLUS:
      case MENU_ENUM_LABEL_R_X_MINUS:
      case MENU_ENUM_LABEL_R_Y_PLUS:
      case MENU_ENUM_LABEL_R_Y_MINUS:
         snprintf(s, len,
               "El eje de un joystick analógico \n"
               "(estilo DualShock).\n"
               " \n"
               "Se asigna como siempre, sin embargo, si se \n"
               "asigna un eje analógico real, puede leerse \n"
               "de forma analógica.\n"
               " \n"
               "El eje X positivo es hacia la derecha. \n"
               "El eje Y positivo es hacia abajo.");
         break;
      case MENU_ENUM_LABEL_VALUE_WHAT_IS_A_CORE_DESC:
         snprintf(s, len,
               "RetroArch, por si solo, no hace nada. \n"
               " \n"
               "Para que haga algo necesitas cargar \n"
               "un programa en él. \n"
               "\n"
               "Llamamos a estos programas 'núcleos \n"
               "libretro', o 'núcleos' para abreviar. \n"
               " \n"
               "Para cargar un núcleo, selecciona uno \n"
               "en 'Cargar núcleo'. \n"
               " \n"
#ifdef HAVE_NETWORKING
               "Puedes conseguir núcleos de varias formas: \n"
               "* Descárgalos en:\n"
               "'%s' -> '%s'.\n"
               "* Cópialos manualmente a:\n"
               "'%s'.",
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_ONLINE_UPDATER),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_CORE_UPDATER_LIST),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_LIBRETRO_DIR_PATH)
#else
               "Puedes conseguir núcleos si los\n"
               "trasladas a mano en la carpeta\n"
               "'%s'.",
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_LIBRETRO_DIR_PATH)
#endif
               );
         break;
      case MENU_ENUM_LABEL_VALUE_HELP_CHANGE_VIRTUAL_GAMEPAD_DESC:
         snprintf(s, len,
               "Puedes cambiar la superposición del \n"
               "mando virtual si vas a '%s' \n"
               "-> '%s'."
               " \n"
               "Desde ahí puedes cambiar la superposición, \n"
               "el tamaño y opacidad de sus botones, etc.\n"
               " \n"
               "NOTA: Las superposiciones de mandos \n"
               "virtuales están ocultas de forma \n"
               "predeterminada si estás dentro del menú. \n"
               "Si quieres cambiar este comportamiento, \n"
               "cambia '%s' a Desactivado/false.",
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_SETTINGS),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_OVERLAY_SETTINGS),
               msg_hash_to_str(MENU_ENUM_LABEL_VALUE_INPUT_OVERLAY_HIDE_IN_MENU)
               );
         break;
      case MSG_UNKNOWN:
      default:
         if (s[0] == '\0')
            strlcpy(s, msg_hash_to_str(MENU_ENUM_LABEL_VALUE_NO_INFORMATION_AVAILABLE), len);
         return -1;
   }

   return 0;
}

const char *msg_hash_to_str_es(enum msg_hash_enums msg)
{
   switch (msg)
   {
      case MENU_ENUM_LABEL_INPUT_MENU_ENUM_TOGGLE_GAMEPAD_COMBO:
         return "Combo para alternar mando con menú";
      case MENU_ENUM_LABEL_MENU_THROTTLE_FRAMERATE:
         return "Acelerar velocidad del menú";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_CHEEVOS_PASSWORD:
         return "Contraseña";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_CHEEVOS_SETTINGS:
         return "Cuenta Cheevos";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_CHEEVOS_USERNAME:
         return "Usuario";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_LIST:
         return "Cuentas";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_LIST_END:
         return "Enlace a lista de cuentas";
      case MENU_ENUM_LABEL_VALUE_ACCOUNTS_RETRO_ACHIEVEMENTS:
         return "Retrologros";
      case MENU_ENUM_LABEL_VALUE_ADD_CONTENT_LIST:
         return "Añadir contenido";
      case MENU_ENUM_LABEL_VALUE_ASK_ARCHIVE:
         return "Preguntar";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS:
         return "Controles básicos del menú";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_BACK:
         return "Retroceder";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_CONFIRM:
         return "Confirmar/Aceptar";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_INFO:
         return "Información";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_QUIT:
         return "Abandonar";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_SCROLL_DOWN:
         return "Desplazar hacia abajo";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_SCROLL_UP:
         return "Desplazar hacia arriba";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_START:
         return "Valores predeterminados";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_TOGGLE_KEYBOARD:
         return "Alternar teclado";
      case MENU_ENUM_LABEL_VALUE_BASIC_MENU_CONTROLS_TOGGLE_MENU:
         return "Alternar menú";
      case MENU_ENUM_LABEL_VALUE_CHEEVOS_DESCRIPTION:
         return "Descripción";
      case MENU_ENUM_LABEL_VALUE_CHEEVOS_HARDCORE_MODE_ENABLE:
         return "Logros Modo Extremo";
      case MENU_ENUM_LABEL_VALUE_CHEEVOS_SETTINGS:
         return "Retrologros";
      case MENU_ENUM_LABEL_VALUE_CHEEVOS_TEST_UNOFFICIAL:
/* FIXME? Translate 'Test Unofficial Achievements' */
         return "Probar versión no oficial";
      case MENU_ENUM_LABEL_VALUE_CONTENT_SETTINGS:
         return "Menú rápido";
      case MENU_ENUM_LABEL_VALUE_DOWNLOAD_CORE_CONTENT:
         return "Descargar contenido";
      case MENU_ENUM_LABEL_VALUE_HELP_AUDIO_VIDEO_TROUBLESHOOTING:
         return "Solucionar problemas de vídeo/sonido";
      case MENU_ENUM_LABEL_VALUE_HELP_CHANGE_VIRTUAL_GAMEPAD:
         return "Cambiar el mando virtual superpuesto";
      case MENU_ENUM_LABEL_VALUE_HELP_CONTROLS:
         return "Controles básicos del menú";
      case MENU_ENUM_LABEL_VALUE_HELP_LIST:
         return "Ayuda";
      case MENU_ENUM_LABEL_VALUE_HELP_LOADING_CONTENT:
         return "Cargando contenidos";
      case MENU_ENUM_LABEL_VALUE_HELP_SCANNING_CONTENT:
         return "Buscar contenido";
      case MENU_ENUM_LABEL_VALUE_HELP_WHAT_IS_A_CORE:
         return "¿Qué es un núcleo?";
      case MENU_ENUM_LABEL_VALUE_INFORMATION_LIST:
         return "Información";
      case MENU_ENUM_LABEL_VALUE_INPUT_ICADE_ENABLE:
         return "Activar asignar mando al teclado";
      case MENU_ENUM_LABEL_VALUE_INPUT_KEYBOARD_GAMEPAD_MAPPING_TYPE:
         return "Tipo de asignación de mando para teclado";
      case MENU_ENUM_LABEL_VALUE_INPUT_OVERLAY_HIDE_IN_MENU:
         return "Ocultar superposición en el menú";
      case MENU_ENUM_LABEL_VALUE_INPUT_SMALL_KEYBOARD_ENABLE:
         return "Activar miniteclado";
      case MENU_ENUM_LABEL_VALUE_LANG_POLISH:
         return "Polaco";
      case MENU_ENUM_LABEL_VALUE_LOAD_ARCHIVE:
         return "Cargar archivo con un núcleo";
      case MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_LIST:
         return "Cargar contenido";
      case MENU_ENUM_LABEL_VALUE_MENU_LINEAR_FILTER:
         return "Filtro lineal del menú";
      case MENU_ENUM_LABEL_VALUE_OPEN_ARCHIVE:
         return "Abrir archivo como una carpeta";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_AUTOLOAD_PREFERRED:
         return "Cargar superposición preferida automáticamente";
      case MENU_ENUM_LABEL_VALUE_PRIVACY_SETTINGS:
         return "Privacidad";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_CRC32:
         return "CRC32";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_MD5:
         return "MD5";
      case MENU_ENUM_LABEL_VALUE_SAVE_CURRENT_CONFIG:
         return "Guardar configuración actual";
      case MENU_ENUM_LABEL_VALUE_SCAN_DIRECTORY:
         return "Escanear carpeta";
      case MENU_ENUM_LABEL_VALUE_SCAN_FILE:
         return "Escanear archivo";
      case MENU_ENUM_LABEL_VALUE_SCAN_THIS_DIRECTORY:
         return "(Escanear esta carpeta)";
      case MENU_ENUM_LABEL_VALUE_STATE_SLOT:
         return "Ranura de guardado";
      case MENU_ENUM_LABEL_VALUE_UPDATE_CORE_INFO_FILES:
         return "Actualizar archivos de información de núcleos";
      case MENU_ENUM_LABEL_VALUE_USE_BUILTIN_PLAYER:
         return "Usar reproductor de medios integrado";
      case MSG_APPENDED_DISK:
         return "Disco incorporado";
      case MSG_APPLYING_SHADER:
         return "Aplicando shader";
      case MSG_AUDIO_MUTED:
         return "Sonido silenciado.";
      case MSG_AUDIO_UNMUTED:
         return "Sonido recuperado.";
      case MSG_AUTOSAVE_FAILED:
         return "No se ha podido iniciar el autoguardado.";
      case MSG_BLOCKING_SRAM_OVERWRITE:
         return "Bloqueando sobrescritura de SRAM";
      case MSG_BYTES:
         return "bytes";
      case MSG_CHEEVOS_HARDCORE_MODE_ENABLE:
         return "Modo Extremo Activado: guardar estado y reboninar se han desactivado.";
      case MSG_CONFIG_DIRECTORY_NOT_SET:
         return "No se ha asignado la carpeta de configuración. No se puede guardar la nueva configuración.";
      case MSG_CORE_DOES_NOT_SUPPORT_SAVESTATES:
         return "El núcleo no es compatible con los guardados rápidos.";
      case MSG_COULD_NOT_READ_CONTENT_FILE:
         return "No se ha podido leer el archivo de contenido";
      case MSG_CUSTOM_TIMING_GIVEN:
         return "Se ha indicado un ritmo personalizado";
      case MSG_DETECTED_VIEWPORT_OF:
         return "Ventana detectada:";
      case MSG_DOWNLOADING:
        return "Descargando";
      case MSG_EXTRACTING:
        return "Extrayendo";
      case MSG_FAILED_TO:
         return "Error:";
      case MSG_FAILED_TO_APPLY_SHADER:
         return "Error al aplicar shader.";
      case MSG_FAILED_TO_LOAD_MOVIE_FILE:
         return "Error al cargar el archivo de película";
      case MSG_FAILED_TO_LOAD_OVERLAY:
         return "Error al cargar sobreimposición.";
      case MSG_FAILED_TO_LOAD_STATE:
         return "Error al cargar rápidamente desde";
      case MSG_FAILED_TO_REMOVE_DISK_FROM_TRAY:
         return "Error al extraer el disco de la bandeja.";
      case MSG_FAILED_TO_REMOVE_TEMPORARY_FILE:
         return "Error al borrar el archivo temporal";
      case MSG_FAILED_TO_SAVE_SRAM:
         return "Error al guardar la SRAM";
      case MSG_FAILED_TO_SAVE_STATE_TO:
         return "Error al guardar rápidamente a";
      case MSG_FAILED_TO_START_MOVIE_RECORD:
         return "Error al iniciar la grabación.";
      case MSG_FAILED_TO_START_RECORDING:
         return "Error al comenzar a grabar.";
      case MSG_FAILED_TO_TAKE_SCREENSHOT:
         return "Error al capturar pantalla.";
      case MSG_FAILED_TO_UNMUTE_AUDIO:
         return "Error al recuperar el sonido.";
      case MSG_FOUND_SHADER:
         return "Shader encontrado";
      case MSG_GOT_INVALID_DISK_INDEX:
         return "Se ha obtenido un índice de disco no válido.";
      case MSG_GRAB_MOUSE_STATE:
         return "Capturar estado de ratón";
      case MSG_HW_RENDERED_MUST_USE_POSTSHADED_RECORDING:
         return "El núcleo Libretro se renderiza por hardware. Es necesario utilizar la grabación post-shaders.";
      case MSG_LIBRETRO_ABI_BREAK:
         return "se ha compilado con una versión distinta a esta implementación de libretro.";
      case MSG_LOADED_STATE_FROM_SLOT:
         return "Carga rápida desde la ranura #%d.";
      case MSG_LOADED_STATE_FROM_SLOT_AUTO:
         return "Carga rápida desde la ranura #-1 (auto).";
      case MSG_LOADING_CONTENT_FILE:
         return "Cargando archivo de contenido";
      case MSG_LOADING_STATE:
         return "Cargando rápidamente";
      case MSG_MOVIE_PLAYBACK_ENDED:
         return "Reproducción detenida.";
      case MSG_MOVIE_RECORD_STOPPED:
         return "Deteniendo grabación de vídeo.";
      case MSG_NETPLAY_FAILED:
         return "Error al iniciar el juego en red.";
      case MSG_PAUSED:
         return "En pausa.";
      case MSG_PROGRAM:
         return "RetroArch";
      case MSG_RECEIVED:
         return "recibido";
      case MSG_RECORDING_TERMINATED_DUE_TO_RESIZE:
         return "Grabación terminada por cambio de tamaño.";
      case MSG_RECORDING_TO:
         return "Grabando a";
      case MSG_REDIRECTING_CHEATFILE_TO:
         return "Redirigiendo archivo de trucos a";
      case MSG_REDIRECTING_SAVEFILE_TO:
         return "Redirigiendo partida guardada a";
      case MSG_REDIRECTING_SAVESTATE_TO:
         return "Redirigiendo guardado rápido a";
      case MSG_REMOVED_DISK_FROM_TRAY:
         return "Se ha retirado el disco de la bandeja.";
      case MSG_REMOVING_TEMPORARY_CONTENT_FILE:
         return "Borrando archivo temporal de contenido";
      case MSG_RESET:
         return "Reiniciar";
      case MSG_RESTARTING_RECORDING_DUE_TO_DRIVER_REINIT:
         return "Reiniciando grabación debido al reinicio de un controlador.";
      case MSG_REWINDING:
         return "Rebobinando.";
      case MSG_REWIND_INIT:
         return "Iniciando búfer de rebobinado, tamaño:";
      case MSG_REWIND_INIT_FAILED:
         return "Error al iniciar el búfer de rebobinado. Rebobinado desactivado.";
      case MSG_REWIND_INIT_FAILED_THREADED_AUDIO:
         return "La implementación utiliza sonido multinúcleo. No se puede utilizar el rebobinado.";
      case MSG_REWIND_REACHED_END:
         return "Se ha llegado al final del búfer de rebobinado.";
      case MSG_SAVED_STATE_TO_SLOT:
         return "Guardando rápidamente a la ranura #%d.";
      case MSG_SAVED_STATE_TO_SLOT_AUTO:
         return "Guardando rápidamente a la ranura #-1 (auto).";
      case MSG_SAVED_SUCCESSFULLY_TO:
         return "Partida guardada en";
      case MSG_SAVING_RAM_TYPE:
         return "Guardando tipo de RAM";
      case MSG_SAVING_STATE:
         return "Guardando rápidamente";
      case MSG_SCANNING:
         return "Escaneando";
      case MSG_SCANNING_OF_DIRECTORY_FINISHED:
         return "Escaneado de carpetas terminado";
      case MSG_SENDING_COMMAND:
         return "Enviando comando";
      case MSG_SHADER:
         return "Shader";
      case MSG_SKIPPING_SRAM_LOAD:
         return "Omitiendo carga de SRAM.";
      case MSG_SLOW_MOTION:
         return "Cámara lenta.";
      case MSG_SLOW_MOTION_REWIND:
         return "Rebobinar cámara lenta.";
      case MSG_SRAM_WILL_NOT_BE_SAVED:
         return "No se guardará la SRAM.";
      case MSG_STARTING_MOVIE_PLAYBACK:
         return "Iniciando reproducción.";
      case MSG_STARTING_MOVIE_RECORD_TO:
         return "Iniciando grabación en";
      case MSG_STATE_SIZE:
         return "Tamaño de guardado rápido";
      case MSG_STATE_SLOT:
         return "Ranura de guardado rápido";
      case MSG_TAKING_SCREENSHOT:
         return "Capturando pantalla.";
      case MSG_TO:
         return "a";
      case MSG_UNKNOWN:
         return "Desconocido";
      case MSG_UNPAUSED:
         return "Sin pausa.";
      case MSG_UNRECOGNIZED_COMMAND:
         return "Comando no reconocido";
      case MSG_USING_LIBRETRO_DUMMY_CORE_RECORDING_SKIPPED:
         return "Se está utilizando el núcleo dummy de libretro. Anulando grabación.";
      case MSG_VIEWPORT_SIZE_CALCULATION_FAILED:
         return "¡Error al calcular el tamaño de ventana! Se utilizarán datos en bruto. Probablemente esto no acabe bien...";
      case MSG_VIRTUAL_DISK_TRAY:
         return "bandeja de disco virtual.";
#if 0
      case MENU_ENUM_LABEL_VALUE_HORIZONTAL_MENU: /* Don't change. Breaks everything. (Would be: "Menú horizontal") */
         return "Horizontal Menu";
         /* Don't change these yet. Breaks everything. */
      case MENU_ENUM_LABEL_VALUE_SETTINGS_TAB:
         return "Pestaña de ajustes";
      case MENU_ENUM_LABEL_VALUE_HISTORY_TAB:
         return "Pestaña de historial";
      case MENU_ENUM_LABEL_VALUE_ADD_TAB:
         return "Añadir pestaña";
      case MENU_ENUM_LABEL_VALUE_PLAYLISTS_TAB:
         return "Pestaña de listas de reproducción";
#endif
      case MENU_ENUM_LABEL_VALUE_ACHIEVEMENT_LIST:
         return "Lista de logros";
      case MENU_ENUM_LABEL_VALUE_ARCHIVE_MODE:
         return "Acción para asociar tipos de archivo";
      case MENU_ENUM_LABEL_VALUE_ASSETS_DIRECTORY:
         return "Carpeta de recursos";
      case MENU_ENUM_LABEL_VALUE_AUDIO_BLOCK_FRAMES:
         return "Bloquear fotogramas";
      case MENU_ENUM_LABEL_VALUE_AUDIO_DEVICE:
         return "Controlador de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_DRIVER:
         return "Controlador de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_DSP_PLUGIN:
         return "Plugin DSP de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_ENABLE:
         return "Activar sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_FILTER_DIR:
         return "Carpeta de filtros de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_LATENCY:
         return "Retraso de sonido (ms)";
      case MENU_ENUM_LABEL_VALUE_AUDIO_MAX_TIMING_SKEW:
         return "Variación máxima de sincronía de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_MUTE:
         return "Silenciar sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_OUTPUT_RATE:
         return "Frecuencia de sonido (KHz)";
      case MENU_ENUM_LABEL_VALUE_AUDIO_RATE_CONTROL_DELTA:
         return "Delta de control de frecuencia de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_RESAMPLER_DRIVER:
         return "Controlador de muestreo de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_SETTINGS:
         return "Sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_SYNC:
         return "Sincronía de sonido";
      case MENU_ENUM_LABEL_VALUE_AUDIO_VOLUME:
         return "Volumen de sonido (dB)";
      case MENU_ENUM_LABEL_VALUE_AUTOSAVE_INTERVAL:
         return "Intervalo de autoguardados de SaveRAM";
      case MENU_ENUM_LABEL_VALUE_AUTO_OVERRIDES_ENABLE:
         return "Cargar autom. archivos de anulación";
      case MENU_ENUM_LABEL_VALUE_AUTO_REMAPS_ENABLE:
         return "Cargar autom. archivos de reasignación";
      case MENU_ENUM_LABEL_VALUE_BLOCK_SRAM_OVERWRITE:
         return "No sobrescribir SaveRAM al cargar un guardado rápido";
      case MENU_ENUM_LABEL_VALUE_BUILDBOT_ASSETS_URL:
         return "URL de recursos de Buildbot";
      case MENU_ENUM_LABEL_VALUE_CACHE_DIRECTORY:
         return "Carpeta de caché";
      case MENU_ENUM_LABEL_VALUE_CAMERA_ALLOW:
         return "Permitir cámara";
      case MENU_ENUM_LABEL_VALUE_CAMERA_DRIVER:
         return "Controlador de cámara";
      case MENU_ENUM_LABEL_VALUE_CHEAT:
         return "Truco";
      case MENU_ENUM_LABEL_VALUE_CHEAT_APPLY_CHANGES:
         return "Aplicar cambios en trucos";
      case MENU_ENUM_LABEL_VALUE_CHEAT_DATABASE_PATH:
         return "Carpeta de archivos de trucos";
      case MENU_ENUM_LABEL_VALUE_CHEAT_FILE_LOAD:
         return "Cargar archivo de trucos";
      case MENU_ENUM_LABEL_VALUE_CHEAT_FILE_SAVE_AS:
         return "Guardar archivo de trucos como...";
      case MENU_ENUM_LABEL_VALUE_CHEAT_NUM_PASSES:
         return "Pasadas de trucos";
      case MENU_ENUM_LABEL_VALUE_CLOSE_CONTENT:
         return "Cerrar";
      case MENU_ENUM_LABEL_VALUE_CONFIGURATIONS:
         return "Cargar configuración";
      case MENU_ENUM_LABEL_VALUE_CONFIGURATION_SETTINGS:
         return "Configuración";
      case MENU_ENUM_LABEL_VALUE_CONFIG_SAVE_ON_EXIT:
         return "Guardar configuración al salir";
      case MENU_ENUM_LABEL_VALUE_CONTENT_COLLECTION_LIST:
         return "Seleccionar de una colección";
      case MENU_ENUM_LABEL_VALUE_CONTENT_DATABASE_DIRECTORY:
         return "Carpeta de bases de datos de contenidos";
      case MENU_ENUM_LABEL_VALUE_CONTENT_HISTORY_SIZE:
         return "Tamaño del historial";
      case MENU_ENUM_LABEL_VALUE_CORE_ASSETS_DIRECTORY:
         return "Carpeta de descargas";
      case MENU_ENUM_LABEL_VALUE_CORE_CHEAT_OPTIONS: /* UPDATE/FIXME */
         return "Opciones de trucos del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_COUNTERS:
         return "Contadores del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_ENABLE:
         return "Mostrar nombre del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_INFORMATION:
         return "Información del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_AUTHORS:
         return "Autores";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_CATEGORIES:
         return "Categorías";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_CORE_LABEL:
         return "Etiqueta del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_CORE_NAME:
         return "Nombre del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_FIRMWARE:
         return "Firmware(s)";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_LICENSES:
         return "Licencia(s)";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_PERMISSIONS:
         return "Permisos";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_SUPPORTED_EXTENSIONS:
         return "Extensiones compatibles";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_SYSTEM_MANUFACTURER:
         return "Fabricante del sistema";
      case MENU_ENUM_LABEL_VALUE_CORE_INFO_SYSTEM_NAME:
         return "Nombre del sistema";
      case MENU_ENUM_LABEL_VALUE_CORE_INPUT_REMAPPING_OPTIONS:
         return "Opciones de entrada del núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_LIST:
         return "Cargar núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_OPTIONS:
         return "Opciones";
      case MENU_ENUM_LABEL_VALUE_CORE_SETTINGS:
         return "Núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_SET_SUPPORTS_NO_CONTENT_ENABLE: /* TODO/FIXME */
         return "No ejecutar automáticamente";
      case MENU_ENUM_LABEL_VALUE_CORE_SPECIFIC_CONFIG:
         return "Configuración por núcleo";
      case MENU_ENUM_LABEL_VALUE_CORE_UPDATER_AUTO_EXTRACT_ARCHIVE:
         return "Extraer automáticamente el archivo descargado";
      case MENU_ENUM_LABEL_VALUE_CORE_UPDATER_BUILDBOT_URL:
         return "URL de núcleos de Buildbot";
      case MENU_ENUM_LABEL_VALUE_CORE_UPDATER_LIST:
         return "Actualizador de núcleos";
      case MENU_ENUM_LABEL_VALUE_CORE_UPDATER_SETTINGS:
         return "Actualizador";
      case MENU_ENUM_LABEL_VALUE_CURSOR_DIRECTORY:
         return "Carpeta de cursores";
      case MENU_ENUM_LABEL_VALUE_CURSOR_MANAGER:
         return "Gestor de cursores";
      case MENU_ENUM_LABEL_VALUE_CUSTOM_RATIO:
         return "Proporción personalizada";
      case MENU_ENUM_LABEL_VALUE_DATABASE_MANAGER:
         return "Gestor de bases de datos";
      case MENU_ENUM_LABEL_VALUE_FAVORITES:
         return "Seleccionar archivo y detectar núcleo"; /* TODO/FIXME - update */
      case MENU_ENUM_LABEL_VALUE_DIRECTORY_CONTENT:
         return "(Carpeta de contenido)";
      case MENU_ENUM_LABEL_VALUE_DIRECTORY_DEFAULT:
         return "(Predeterminada)";
      case MENU_ENUM_LABEL_VALUE_DIRECTORY_NONE:
         return "(Ninguna)";
      case MENU_ENUM_LABEL_VALUE_DIRECTORY_NOT_FOUND:
         return "No se ha encontrado la carpeta.";
      case MENU_ENUM_LABEL_VALUE_DIRECTORY_SETTINGS:
         return "Carpeta";
      case MENU_ENUM_LABEL_VALUE_DISABLED:
         return "Desactivado";
      case MENU_ENUM_LABEL_VALUE_DISK_CYCLE_TRAY_STATUS:
         return "Estado de la bandeja del disco";
      case MENU_ENUM_LABEL_VALUE_DISK_IMAGE_APPEND:
         return "Asignar imagen de disco";
      case MENU_ENUM_LABEL_VALUE_DISK_INDEX:
         return "Índice del disco";
      case MENU_ENUM_LABEL_VALUE_DISK_OPTIONS: /* UPDATE/FIXME */
         return "Opciones del disco del núcleo";
      case MENU_ENUM_LABEL_VALUE_DONT_CARE:
         return "No importa";
      case MENU_ENUM_LABEL_VALUE_DOWNLOADED_FILE_DETECT_CORE_LIST: /* TODO/FIXME - rewrite */
         return "Seleccionar archivo descargado y detectar núcleo";
      case MENU_ENUM_LABEL_VALUE_DPI_OVERRIDE_ENABLE:
         return "Activar anulación de PPP";
      case MENU_ENUM_LABEL_VALUE_DPI_OVERRIDE_VALUE:
         return "Anular PPP";
      case MENU_ENUM_LABEL_VALUE_DRIVER_SETTINGS:
         return "Controlador";
      case MENU_ENUM_LABEL_VALUE_DUMMY_ON_CORE_SHUTDOWN:
         return "Anular al cerrar núcleo";
      case MENU_ENUM_LABEL_VALUE_DYNAMIC_WALLPAPER:
         return "Fondo de pantalla dinámico";
      case MENU_ENUM_LABEL_VALUE_DYNAMIC_WALLPAPERS_DIRECTORY:
         return "Carpeta de fondos de pantalla dinámicos";
      case MENU_ENUM_LABEL_VALUE_CHEEVOS_ENABLE:
/* FIXME? Translate 'Enable Achievements' */
         return "Activar";
      case MENU_ENUM_LABEL_VALUE_ENTRY_HOVER_COLOR:
         return "Color de entrada resaltada del menú";
      case MENU_ENUM_LABEL_VALUE_ENTRY_NORMAL_COLOR:
         return "Color de entrada normal del menú";
      case MENU_ENUM_LABEL_VALUE_FALSE:
         return "Desactivado";
      case MENU_ENUM_LABEL_VALUE_FASTFORWARD_RATIO:
         return "Velocidad máxima de ejecución";
      case MENU_ENUM_LABEL_VALUE_FPS_SHOW:
         return "Mostrar velocidad de fotogramas";
      case MENU_ENUM_LABEL_VALUE_FRAME_THROTTLE_ENABLE:
         return "Limitar velocidad máxima de ejecución";
      case MENU_ENUM_LABEL_VALUE_FRAME_THROTTLE_SETTINGS:
         return "Aumento de fotogramas";
      case MENU_ENUM_LABEL_VALUE_FRONTEND_COUNTERS:
         return "Contadores del frontend";
      case MENU_ENUM_LABEL_VALUE_GAME_SPECIFIC_OPTIONS:
         return "Usar opciones de núcleo para cada juego si existen";
      case MENU_ENUM_LABEL_VALUE_GAME_SPECIFIC_OPTIONS_CREATE:
         return "Crear archivo de opciones del juego";
      case MENU_ENUM_LABEL_VALUE_GAME_SPECIFIC_OPTIONS_IN_USE:
         return "Archivo de opciones del juego";
      case MENU_ENUM_LABEL_VALUE_HELP:
         return "Ayuda";
      case MENU_ENUM_LABEL_VALUE_HISTORY_LIST_ENABLE:
         return "Activar historial";
      case MENU_ENUM_LABEL_VALUE_INPUT_AUTODETECT_ENABLE:
         return "Activar autoconfiguración";
      case MENU_ENUM_LABEL_VALUE_INPUT_AXIS_THRESHOLD:
         return "Margen de ejes de entrada";
      case MENU_ENUM_LABEL_VALUE_INPUT_DESCRIPTOR_HIDE_UNBOUND:
         return "Ocultar descripciones sin asignar de la entrada del núcleo";
      case MENU_ENUM_LABEL_VALUE_INPUT_DESCRIPTOR_LABEL_SHOW:
         return "Mostrar etiquetas de descripción de la entrada";
      case MENU_ENUM_LABEL_VALUE_INPUT_DRIVER:
         return "Controlador de entrada";
      case MENU_ENUM_LABEL_VALUE_INPUT_DUTY_CYCLE:
         return "Ciclo de trabajo";
      case MENU_ENUM_LABEL_VALUE_INPUT_HOTKEY_BINDS:
         return "Asignaciones de teclas rápidas";
      case MENU_ENUM_LABEL_VALUE_INPUT_MAX_USERS:
         return "Número máximo de usuarios";
      case MENU_ENUM_LABEL_VALUE_INPUT_OSK_OVERLAY_ENABLE:
         return "Mostrar teclado superpuesto";
      case MENU_ENUM_LABEL_VALUE_INPUT_OVERLAY_ENABLE:
         return "Mostrar superposición";
      case MENU_ENUM_LABEL_VALUE_INPUT_POLL_TYPE_BEHAVIOR:
         return "Tipo de retardo";
      case MENU_ENUM_LABEL_VALUE_INPUT_REMAPPING_DIRECTORY:
         return "Carpeta de reasignación de entrada";
      case MENU_ENUM_LABEL_VALUE_INPUT_REMAP_BINDS_ENABLE:
         return "Permitir reasignar controles";
      case MENU_ENUM_LABEL_VALUE_INPUT_SETTINGS:
         return "Entrada";
      case MENU_ENUM_LABEL_VALUE_INPUT_TURBO_PERIOD:
         return "Período de turbo";
      case MENU_ENUM_LABEL_VALUE_INPUT_USER_BINDS:
         return "Asignaciones de entrada del usuario %u";
      case MENU_ENUM_LABEL_VALUE_JOYPAD_AUTOCONFIG_DIR:
         return "Carpeta de autoconfiguración de dispositivo de entrada";
      case MENU_ENUM_LABEL_VALUE_JOYPAD_DRIVER:
         return "Controlador de joypad";
      case MENU_ENUM_LABEL_VALUE_LANG_CHINESE_SIMPLIFIED:
         return "Chino (Simplificado)";
      case MENU_ENUM_LABEL_VALUE_LANG_CHINESE_TRADITIONAL:
         return "Chino (Tradicional)";
      case MENU_ENUM_LABEL_VALUE_LANG_DUTCH:
         return "Holandés";
      case MENU_ENUM_LABEL_VALUE_LANG_ENGLISH:
         return "Inglés";
      case MENU_ENUM_LABEL_VALUE_LANG_ESPERANTO:
         return "Esperanto";
      case MENU_ENUM_LABEL_VALUE_LANG_FRENCH:
         return "Francés";
      case MENU_ENUM_LABEL_VALUE_LANG_GERMAN:
         return "Alemán";
      case MENU_ENUM_LABEL_VALUE_LANG_ITALIAN:
         return "Italiano";
      case MENU_ENUM_LABEL_VALUE_LANG_JAPANESE:
         return "Japonés";
      case MENU_ENUM_LABEL_VALUE_LANG_KOREAN:
         return "Coreano";
      case MENU_ENUM_LABEL_VALUE_LANG_PORTUGUESE:
         return "Portugués";
      case MENU_ENUM_LABEL_VALUE_LANG_RUSSIAN:
         return "Ruso";
      case MENU_ENUM_LABEL_VALUE_LANG_SPANISH:
         return "Español";
      case MENU_ENUM_LABEL_VALUE_LEFT_ANALOG:
         return "Analógico izquierdo";
      case MENU_ENUM_LABEL_VALUE_LIBRETRO_DIR_PATH:
         return "Carpeta de núcleos";
      case MENU_ENUM_LABEL_VALUE_LIBRETRO_INFO_PATH:
         return "Carpeta de información del núcleo";
      case MENU_ENUM_LABEL_VALUE_LIBRETRO_LOG_LEVEL:
         return "Nivel de registro del núcleo";
      case MENU_ENUM_LABEL_VALUE_LINEAR:
         return "Lineal";
      case MENU_ENUM_LABEL_VALUE_LOAD_CONTENT_HISTORY:
         return "Cargar archivos recientes";
      case MENU_ENUM_LABEL_VALUE_LOAD_STATE:
         return "Carga rápida";
      case MENU_ENUM_LABEL_VALUE_LOCATION_ALLOW:
         return "Permitir ubicación";
      case MENU_ENUM_LABEL_VALUE_LOCATION_DRIVER:
         return "Controlador de ubicación";
      case MENU_ENUM_LABEL_VALUE_LOGGING_SETTINGS:
         return "Registros";
      case MENU_ENUM_LABEL_VALUE_LOG_VERBOSITY:
         return "Verbosidad del registro";
      case MENU_ENUM_LABEL_VALUE_MAIN_MENU:
         return "Menú principal";
      case MENU_ENUM_LABEL_VALUE_MANAGEMENT:
         return "Ajustes de bases de datos";
      case MENU_ENUM_LABEL_VALUE_MENU_DRIVER:
         return "Controlador de menú";
      case MENU_ENUM_LABEL_VALUE_MENU_FILE_BROWSER_SETTINGS:
         return "Navegador de archivos del menú";
      case MENU_ENUM_LABEL_VALUE_MENU_SETTINGS:
         return "Menú";
      case MENU_ENUM_LABEL_VALUE_MENU_WALLPAPER:
         return "Fondo del menú";
      case MENU_ENUM_LABEL_VALUE_MISSING:
         return "Desaparecido";
      case MENU_ENUM_LABEL_VALUE_MOUSE_ENABLE:
         return "Soporte para teclado";
      case MENU_ENUM_LABEL_VALUE_MULTIMEDIA_SETTINGS:
         return "Multimedia";
      case MENU_ENUM_LABEL_VALUE_NAVIGATION_BROWSER_FILTER_SUPPORTED_EXTENSIONS_ENABLE:
         return "Filtrar por extensiones compatibles"; /* TODO/FIXME - rewrite */
      case MENU_ENUM_LABEL_VALUE_NAVIGATION_WRAPAROUND:
         return "Seguir navegación";
      case MENU_ENUM_LABEL_VALUE_NEAREST:
         return "Más cercano";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_CLIENT_SWAP_INPUT: /* TODO: Original string changed */
         return "Intercambiar entrada en red";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_DELAY_FRAMES:
         return "Retraso de fotogramas en red";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_ENABLE:
         return "Activar juego en red";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_IP_ADDRESS: /* TODO: Original string changed */
         return "Dirección IP";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_MODE:
         return "Activar cliente en red";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_NICKNAME:
         return "Nombre de usuario";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_SPECTATOR_MODE_ENABLE:
         return "Permitir espectadores en red";
      case MENU_ENUM_LABEL_VALUE_NETPLAY_TCP_UDP_PORT:
         return "Puerto TCP/UDP para juego en red";
      case MENU_ENUM_LABEL_VALUE_NETWORK_CMD_ENABLE:
         return "Comandos de red";
      case MENU_ENUM_LABEL_VALUE_NETWORK_CMD_PORT:
         return "Puerto de comandos de red";
      case MENU_ENUM_LABEL_VALUE_NETWORK_REMOTE_ENABLE:
         return "Mando en red";
      case MENU_ENUM_LABEL_VALUE_NETWORK_SETTINGS:
         return "Red";
      case MENU_ENUM_LABEL_VALUE_NO:
         return "No";
      case MENU_ENUM_LABEL_VALUE_NONE:
         return "Ninguno";
      case MENU_ENUM_LABEL_VALUE_NOT_AVAILABLE:
         return "No disponible";
      case MENU_ENUM_LABEL_VALUE_NO_CORE:
         return "Sin núcleo";
      case MENU_ENUM_LABEL_VALUE_NO_CORES_AVAILABLE:
         return "No hay núcleos.";
      case MENU_ENUM_LABEL_VALUE_NO_CORE_INFORMATION_AVAILABLE:
         return "No hay información del núcleo.";
      case MENU_ENUM_LABEL_VALUE_NO_CORE_OPTIONS_AVAILABLE:
         return "No hay opciones del núcleo.";
      case MENU_ENUM_LABEL_VALUE_NO_INFORMATION_AVAILABLE:
         return "No hay información disponible.";
      case MENU_ENUM_LABEL_VALUE_NO_ITEMS:
         return "No hay elementos.";
      case MENU_ENUM_LABEL_VALUE_NO_PERFORMANCE_COUNTERS:
         return "No hay contadores de rendimiento.";
      case MENU_ENUM_LABEL_VALUE_NO_PLAYLIST_ENTRIES_AVAILABLE:
         return "No hay listas de reproducción.";
      case MENU_ENUM_LABEL_VALUE_NO_SETTINGS_FOUND:
         return "No se ha encontrado una configuración.";
      case MENU_ENUM_LABEL_VALUE_NO_SHADER_PARAMETERS:
         return "No hay parámetros de shaders.";
      case MENU_ENUM_LABEL_VALUE_OFF: /* Not changed. Would be "SÍ" */
         return "OFF";
      case MENU_ENUM_LABEL_VALUE_ON: /* Not changed. Would be "NO" */
         return "ON";
      case MENU_ENUM_LABEL_VALUE_ONLINE_UPDATER:
         return "Actualizador en línea";
      case MENU_ENUM_LABEL_VALUE_ONSCREEN_DISPLAY_SETTINGS:
         return "Textos en pantalla (OSD)";
      case MENU_ENUM_LABEL_VALUE_OPTIONAL:
         return "Opcional";
      case MENU_ENUM_LABEL_VALUE_OSK_OVERLAY_DIRECTORY:
         return "Carpeta de teclados superpuestos";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_DIRECTORY:
         return "Carpeta de superposiciones";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_OPACITY:
         return "Opacidad de la superposición";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_PRESET:
         return "Preajuste de superposición";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_SCALE:
         return "Escala de superposición";
      case MENU_ENUM_LABEL_VALUE_OVERLAY_SETTINGS:
         return "Superposición";
      case MENU_ENUM_LABEL_VALUE_PAL60_ENABLE:
         return "Usar modo PAL60";
      case MENU_ENUM_LABEL_VALUE_PAUSE_LIBRETRO:
         return "Pausar al activar el menú";
      case MENU_ENUM_LABEL_VALUE_PAUSE_NONACTIVE:
         return "Detenerse en segundo plano";
      case MENU_ENUM_LABEL_VALUE_PERFCNT_ENABLE:
         return "Contadores de rendimiento";
      case MENU_ENUM_LABEL_VALUE_PLAYLIST_DIRECTORY:
         return "Carpeta de listas de reproducción";
      case MENU_ENUM_LABEL_VALUE_PLAYLIST_SETTINGS:
         return "Lista de reproducción";
      case MENU_ENUM_LABEL_VALUE_POINTER_ENABLE:
         return "Soporte táctil";
      case MENU_ENUM_LABEL_VALUE_PORT:
         return "Puerto";
      case MENU_ENUM_LABEL_VALUE_PRESENT:
         return "Presente";
      case MENU_ENUM_LABEL_VALUE_QUIT_RETROARCH:
         return "Abandonar RetroArch";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_DESCRIPTION:
         return "Descripción";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_DEVELOPER:
         return "Desarrollador";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_FRANCHISE:
         return "Franquicia";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_GENRE:
         return "Género";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_NAME:
         return "Nombre";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_ORIGIN:
         return "Origen";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_PUBLISHER:
         return "Distribuidora";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_RELEASE_MONTH:
         return "Mes de lanzamiento";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_RELEASE_YEAR:
         return "Año de lanzamiento";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_SHA1:
         return "SHA1";
      case MENU_ENUM_LABEL_VALUE_RDB_ENTRY_START_CONTENT:
         return "Ejecutar contenido";
      case MENU_ENUM_LABEL_VALUE_REBOOT:
         return "Reiniciar";
      case MENU_ENUM_LABEL_VALUE_RECORDING_CONFIG_DIRECTORY:
         return "Carpeta de configuración de grabación";
      case MENU_ENUM_LABEL_VALUE_RECORDING_OUTPUT_DIRECTORY:
         return "Carpeta de salida de grabación";
      case MENU_ENUM_LABEL_VALUE_RECORDING_SETTINGS:
         return "Grabación";
      case MENU_ENUM_LABEL_VALUE_RECORD_CONFIG:
         return "Configuración de grabación";
      case MENU_ENUM_LABEL_VALUE_RECORD_DRIVER:
         return "Controlador de grabación";
      case MENU_ENUM_LABEL_VALUE_RECORD_ENABLE:
         return "Activar grabación";
      case MENU_ENUM_LABEL_VALUE_RECORD_PATH:
         return "Carpeta de salida";
      case MENU_ENUM_LABEL_VALUE_RECORD_USE_OUTPUT_DIRECTORY:
         return "Usar carpeta de salida";
      case MENU_ENUM_LABEL_VALUE_REMAP_FILE_LOAD:
         return "Cargar archivo de reasignación";
      case MENU_ENUM_LABEL_VALUE_REMAP_FILE_SAVE_CORE:
         return "Guardar archivo de reasignación del núcleo";
      case MENU_ENUM_LABEL_VALUE_REMAP_FILE_SAVE_GAME:
         return "Guardar archivo de reasignación del juego";
      case MENU_ENUM_LABEL_VALUE_REQUIRED:
         return "Necesario";
      case MENU_ENUM_LABEL_VALUE_RESTART_CONTENT:
         return "Reiniciar";
      case MENU_ENUM_LABEL_VALUE_RESTART_RETROARCH:
         return "Reiniciar RetroArch";
      case MENU_ENUM_LABEL_VALUE_RESUME:
         return "Reanudar";
      case MENU_ENUM_LABEL_VALUE_RESUME_CONTENT:
         return "Reanudar";
      case MENU_ENUM_LABEL_VALUE_RETROKEYBOARD:
         return "RetroKeyboard";
      case MENU_ENUM_LABEL_VALUE_RETROPAD:
         return "RetroPad";
      case MENU_ENUM_LABEL_VALUE_REWIND_ENABLE:
         return "Activar rebobinado";
      case MENU_ENUM_LABEL_VALUE_REWIND_GRANULARITY:
         return "Nivel de detalle del rebobinado";
      case MENU_ENUM_LABEL_VALUE_REWIND_SETTINGS:
         return "Rebobinado";
      case MENU_ENUM_LABEL_VALUE_RGUI_BROWSER_DIRECTORY:
         return "Carpeta del navegador de archivos";
      case MENU_ENUM_LABEL_VALUE_RGUI_CONFIG_DIRECTORY:
         return "Carpeta de configuración";
      case MENU_ENUM_LABEL_VALUE_RGUI_SHOW_START_SCREEN:
         return "Mostrar pantalla de inicio";
      case MENU_ENUM_LABEL_VALUE_RIGHT_ANALOG:
         return "Analógico derecho";
      case MENU_ENUM_LABEL_VALUE_SAVEFILE_DIRECTORY:
         return "Carpeta de partidas guardadas";
      case MENU_ENUM_LABEL_VALUE_SAVESTATE_AUTO_INDEX:
         return "Indizar automáticamente guardados rápidos";
      case MENU_ENUM_LABEL_VALUE_SAVESTATE_AUTO_LOAD:
         return "Cargar guardado rápido automáticamente";
      case MENU_ENUM_LABEL_VALUE_SAVESTATE_AUTO_SAVE:
         return "Guardado rápido automático";
      case MENU_ENUM_LABEL_VALUE_SAVESTATE_DIRECTORY:
         return "Carpeta de guardados rápidos";
      case MENU_ENUM_LABEL_VALUE_SAVE_NEW_CONFIG:
         return "Guardar configuración nueva";
      case MENU_ENUM_LABEL_VALUE_SAVE_STATE:
         return "Guardado rápido";
      case MENU_ENUM_LABEL_VALUE_SAVING_SETTINGS:
         return "Guardado";
      case MENU_ENUM_LABEL_VALUE_SCREENSHOT_DIRECTORY:
         return "Carpeta de capturas de pantalla";
      case MENU_ENUM_LABEL_VALUE_SCREEN_RESOLUTION:
         return "Resolución de pantalla";
      case MENU_ENUM_LABEL_VALUE_SEARCH:
         return "Buscar:";
      case MENU_ENUM_LABEL_VALUE_SECONDS:
         return "segundos";
      case MENU_ENUM_LABEL_VALUE_SETTINGS:
         return "Ajustes";
      case MENU_ENUM_LABEL_VALUE_SHADER:
         return "Shader";
      case MENU_ENUM_LABEL_VALUE_SHADER_APPLY_CHANGES:
         return "Aplicar cambios en shaders";
      case MENU_ENUM_LABEL_VALUE_SHADER_OPTIONS:
         return "Opciones de shaders";
      case MENU_ENUM_LABEL_VALUE_SHOW_ADVANCED_SETTINGS:
         return "Mostrar ajustes avanzados";
      case MENU_ENUM_LABEL_VALUE_SHUTDOWN:
         return "Apagar";
      case MENU_ENUM_LABEL_VALUE_SLOWMOTION_RATIO:
         return "Cantidad de velocidad reducida";
      case MENU_ENUM_LABEL_VALUE_SORT_SAVEFILES_ENABLE:
         return "Ordenar partidas guardadas por carpetas";
      case MENU_ENUM_LABEL_VALUE_SORT_SAVESTATES_ENABLE:
         return "Ordenar guardados rápidos por carpetas";
      case MENU_ENUM_LABEL_VALUE_START_CORE:
         return "Iniciar núcleo";
      case MENU_ENUM_LABEL_VALUE_STATUS:
         return "Estado";
      case MENU_ENUM_LABEL_VALUE_STDIN_CMD_ENABLE:
         return "Comandos stdin";
      case MENU_ENUM_LABEL_VALUE_SUSPEND_SCREENSAVER_ENABLE:
         return "Suspender salvapantallas";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_BGM_ENABLE:
         return "Activar música del sistema";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_DIRECTORY:
         return "Carpeta de sistema/BIOS";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFORMATION:
         return "Información del sistema";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_7ZIP_SUPPORT:
         return "Soporte de 7zip";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_ALSA_SUPPORT:
         return "Soporte de ALSA";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_BUILD_DATE:
         return "Fecha de compilación";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_CG_SUPPORT:
         return "Soporte de Cg";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_COCOA_SUPPORT:
         return "Soporte de Cocoa";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_COMMAND_IFACE_SUPPORT:
         return "Soporte de interfaz de comandos";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_CORETEXT_SUPPORT:
         return "Soporte de CoreText";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_CPU_FEATURES:
         return "Características de CPU";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DISPLAY_METRIC_DPI:
         return "Mostrar PPP métricos";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DISPLAY_METRIC_MM_HEIGHT:
         return "Mostrar alto métrico (mm)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DISPLAY_METRIC_MM_WIDTH:
         return "Mostrar ancho métrico (mm)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DSOUND_SUPPORT:
         return "Soporte de DirectSound";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DYLIB_SUPPORT:
         return "Soporte de librerías dinámicas";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_DYNAMIC_SUPPORT:
         return "Carga dinámica en tiempo real de librería libretro";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_EGL_SUPPORT:
         return "Soporte de EGL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FBO_SUPPORT:
         return "Soporte de render-to-texture OpenGL/Direct3D (shaders multipasos)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FFMPEG_SUPPORT:
         return "Soporte de FFmpeg";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FREETYPE_SUPPORT:
         return "Soporte de FreeType";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FRONTEND_IDENTIFIER:
         return "Identificador del frontend";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FRONTEND_NAME:
         return "Nombre del frontend";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_FRONTEND_OS:
         return "S.O. del frontend";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_GIT_VERSION:
         return "Versión de Git";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_GLSL_SUPPORT:
         return "Soporte de GLSL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_HLSL_SUPPORT:
         return "Soporte de HLSL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_JACK_SUPPORT:
         return "Soporte de JACK";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_KMS_SUPPORT:
         return "Soporte de KMS/EGL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_LIBRETRODB_SUPPORT:
         return "Soporte de LibretroDB";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_LIBUSB_SUPPORT:
         return "Soporte de Libusb";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_LIBXML2_SUPPORT:
         return "Soporte de parseo XML libxml2";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_NETPLAY_SUPPORT:
         return "Soporte de juego en red (peer-to-peer)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_NETWORK_COMMAND_IFACE_SUPPORT:
         return "Soporte de interfaz de comandos en red";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_NETWORK_REMOTE_SUPPORT:
         return "Soporte de mando en red";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OPENAL_SUPPORT:
         return "Soporte de OpenAL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OPENGLES_SUPPORT:
         return "Soporte de OpenGL ES";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OPENGL_SUPPORT:
         return "Soporte de OpenGL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OPENSL_SUPPORT:
         return "Soporte de OpenSL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OPENVG_SUPPORT:
         return "Soporte de OpenVG";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OSS_SUPPORT:
         return "Soporte de OSS";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_OVERLAY_SUPPORT:
         return "Soporte de superposiciones";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_POWER_SOURCE:
         return "Fuente de alimentación";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_POWER_SOURCE_CHARGED:
         return "Cargada";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_POWER_SOURCE_CHARGING:
         return "Cargando";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_POWER_SOURCE_DISCHARGING:
         return "Descargando";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_POWER_SOURCE_NO_SOURCE:
         return "No hay una fuente";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_PULSEAUDIO_SUPPORT:
         return "Soporte de PulseAudio";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_PYTHON_SUPPORT:
         return "Soporte de Python (soporte de scripts para shaders)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_RETRORATING_LEVEL:
         return "Nivel de RetroRating";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_ROARAUDIO_SUPPORT:
         return "Soporte de RoarAudio";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_RPNG_SUPPORT:
         return "Soporte de PNG (RPNG)";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_RSOUND_SUPPORT:
         return "Soporte de RSound";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_SDL2_SUPPORT:
         return "Soporte de SDL2";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_SDL_IMAGE_SUPPORT:
         return "Soporte de imágenes SDL";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_SDL_SUPPORT:
         return "Soporte de SDL1.2";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_THREADING_SUPPORT:
         return "Soporte de hilos";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_UDEV_SUPPORT:
         return "Soporte de Udev";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_V4L2_SUPPORT:
         return "Soporte de Video4Linux2";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_VIDEO_CONTEXT_DRIVER:
         return "Controlador de contexto de vídeo";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_VULKAN_SUPPORT:
         return "Soporte Vulkan";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_WAYLAND_SUPPORT:
         return "Soporte de Wayland";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_X11_SUPPORT:
         return "Soporte de X11";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_XAUDIO2_SUPPORT:
         return "Soporte de XAudio2";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_XVIDEO_SUPPORT:
         return "Soporte de XVideo";
      case MENU_ENUM_LABEL_VALUE_SYSTEM_INFO_ZLIB_SUPPORT:
         return "Soporte de Zlib";
      case MENU_ENUM_LABEL_VALUE_TAKE_SCREENSHOT:
         return "Capturar pantalla";
      case MENU_ENUM_LABEL_VALUE_THREADED_DATA_RUNLOOP_ENABLE:
         return "Buclar datos hilados";
      case MENU_ENUM_LABEL_VALUE_TIMEDATE_ENABLE:
         return "Mostrar fecha y hora";
      case MENU_ENUM_LABEL_VALUE_TITLE_COLOR:
         return "Color de títulos del menú";
      case MENU_ENUM_LABEL_VALUE_TRUE:
         return "Activado";
      case MENU_ENUM_LABEL_VALUE_UI_COMPANION_ENABLE:
         return "Activar IU ayudante";
      case MENU_ENUM_LABEL_VALUE_UI_COMPANION_START_ON_BOOT:
         return "Ejecutar al inicio la IU ayudante";
      case MENU_ENUM_LABEL_VALUE_UI_MENUBAR_ENABLE:
         return "Barra de menús";
      case MENU_ENUM_LABEL_VALUE_UNABLE_TO_READ_COMPRESSED_FILE:
         return "No se ha podido leer el archivo comprimido.";
      case MENU_ENUM_LABEL_VALUE_UNKNOWN:
         return "Desconocido";
      case MENU_ENUM_LABEL_VALUE_UPDATE_ASSETS:
         return "Actualizar recursos";
      case MENU_ENUM_LABEL_VALUE_UPDATE_AUTOCONFIG_PROFILES:
         return "Actualizar perfiles de autoconfiguración";
      case MENU_ENUM_LABEL_VALUE_UPDATE_CG_SHADERS:
         return "Actualizar shaders Cg";
      case MENU_ENUM_LABEL_VALUE_UPDATE_CHEATS:
         return "Actualizar trucos";
      case MENU_ENUM_LABEL_VALUE_UPDATE_DATABASES:
         return "Actualizar bases de datos";
      case MENU_ENUM_LABEL_VALUE_UPDATE_GLSL_SHADERS:
         return "Actualizar shaders GLSL";
      case MENU_ENUM_LABEL_VALUE_UPDATE_LAKKA:
         return "Actualizar Lakka";
      case MENU_ENUM_LABEL_VALUE_UPDATE_OVERLAYS:
         return "Actualizar superposiciones";
      case MENU_ENUM_LABEL_VALUE_USER:
         return "Usuario";
      case MENU_ENUM_LABEL_VALUE_USER_INTERFACE_SETTINGS:
         return "Interfaz de usuario";
      case MENU_ENUM_LABEL_VALUE_USER_LANGUAGE:
         return "Idioma";
      case MENU_ENUM_LABEL_VALUE_USER_SETTINGS:
         return "Usuario";
      case MENU_ENUM_LABEL_VALUE_USE_BUILTIN_IMAGE_VIEWER:
         return "Usar visualizador de imágenes integrado";
      case MENU_ENUM_LABEL_VALUE_USE_THIS_DIRECTORY:
         return "(Usar esta carpeta)";
      case MENU_ENUM_LABEL_VALUE_VIDEO_ALLOW_ROTATE:
         return "Permitir rotación";
      case MENU_ENUM_LABEL_VALUE_VIDEO_ASPECT_RATIO_AUTO:
         return "Proporción de aspecto automática";
      case MENU_ENUM_LABEL_VALUE_VIDEO_ASPECT_RATIO_INDEX:
         return "Índice de proporción de aspecto";
      case MENU_ENUM_LABEL_VALUE_VIDEO_BLACK_FRAME_INSERTION:
         return "Insertar fotogramas negros";
      case MENU_ENUM_LABEL_VALUE_VIDEO_CROP_OVERSCAN:
         return "Recortar Overscan (Reinicio)";
      case MENU_ENUM_LABEL_VALUE_VIDEO_DISABLE_COMPOSITION:
         return "Desactivar composición de escritorio";
      case MENU_ENUM_LABEL_VALUE_VIDEO_DRIVER:
         return "Controlador de vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FILTER:
         return "Filtro de vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FILTER_DIR:
         return "Carpeta de filtros de vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FILTER_FLICKER:
         return "Filtro de parpadeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FONT_ENABLE:
         return "Mostrar mensajes en pantalla";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FONT_PATH:
         return "Fuente de mensajes en pantalla";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FONT_SIZE:
         return "Tamaño de mensajes en pantalla";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FORCE_ASPECT:
         return "Forzar proporción de aspecto";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FORCE_SRGB_DISABLE:
         return "Forzar anulación del FBO sRGB";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FRAME_DELAY:
         return "Retraso de fotogramas";
      case MENU_ENUM_LABEL_VALUE_VIDEO_FULLSCREEN:
         return "Pantalla completa";
      case MENU_ENUM_LABEL_VALUE_VIDEO_GAMMA:
         return "Gamma de vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_GPU_RECORD:
         return "Activar grabación de GPU";
      case MENU_ENUM_LABEL_VALUE_VIDEO_GPU_SCREENSHOT:
         return "Permitir capturas de pantalla de GPU";
      case MENU_ENUM_LABEL_VALUE_VIDEO_HARD_SYNC:
         return "Sincronía estricta de GPU";
      case MENU_ENUM_LABEL_VALUE_VIDEO_HARD_SYNC_FRAMES:
         return "Fotogramas a sincronizar estrictamente";
      case MENU_ENUM_LABEL_VALUE_VIDEO_MESSAGE_POS_X:
         return "Posición X de mensajes en pantalla";
      case MENU_ENUM_LABEL_VALUE_VIDEO_MESSAGE_POS_Y:
         return "Posición Y de mensajes en pantalla";
      case MENU_ENUM_LABEL_VALUE_VIDEO_MONITOR_INDEX:
         return "Índice del monitor";
      case MENU_ENUM_LABEL_VALUE_VIDEO_POST_FILTER_RECORD:
         return "Activar grabación con filtros";
      case MENU_ENUM_LABEL_VALUE_VIDEO_REFRESH_RATE:
         return "Frecuencia de actualización";
      case MENU_ENUM_LABEL_VALUE_VIDEO_REFRESH_RATE_AUTO:
         return "Frecuencia estimada del monitor";
      case MENU_ENUM_LABEL_VALUE_VIDEO_ROTATION:
         return "Rotación";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SCALE:
         return "Escala de ventana";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SCALE_INTEGER:
         return "Escala integral";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SETTINGS:
         return "Vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_DIR:
         return "Carpeta de shaders de vídeo";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_NUM_PASSES:
         return "Pasadas del shader";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PARAMETERS:
         return "Previsualizar parámetros de shaders";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PRESET:
         return "Cargar preajuste de shaders";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PRESET_PARAMETERS:
         return "Parámetros de shaders del menú";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHADER_PRESET_SAVE_AS:
         return "Guardar preajuste de shaders como...";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SHARED_CONTEXT:
         return "Activar contexto compartido por HW";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SMOOTH:
         return "Filtrado bilineal por hardware";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SOFT_FILTER:
         return "Activar filtros por software";
      case MENU_ENUM_LABEL_VALUE_VIDEO_SWAP_INTERVAL:
         return "Intervalo de alternado de VSync";
      case MENU_ENUM_LABEL_VALUE_VIDEO_THREADED:
         return "Vídeo por hilos";
      case MENU_ENUM_LABEL_VALUE_VIDEO_VFILTER:
         return "Filtro contra parpadeos";
      case MENU_ENUM_LABEL_VALUE_VIDEO_VI_WIDTH:
         return "Asignar ancho de interfaz visual";
      case MENU_ENUM_LABEL_VALUE_VIDEO_VSYNC:
         return "VSync/Sincronía vertical";
      case MENU_ENUM_LABEL_VALUE_VIDEO_WINDOWED_FULLSCREEN:
         return "Pantalla completa en ventana";
      case MENU_ENUM_LABEL_VALUE_XMB_ALPHA_FACTOR:
         return "Transparencia del XMB";
      case MENU_ENUM_LABEL_VALUE_XMB_FONT:
         return "Fuente del XMB";
      case MENU_ENUM_LABEL_VALUE_XMB_SCALE_FACTOR:
         return "Escala del XMB";
      case MENU_ENUM_LABEL_VALUE_YES:
         return "Sí";
      default:
         break;
   }

   return "null";
}

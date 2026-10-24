# Recursos del juego

Solo `runtime/` se copia al ejecutable y al paquete portable. `runtime/catalog.json` relaciona cada nombre original con su ruta actual.

| Carpeta runtime | Contenido |
| --- | --- |
| characters | Habitantes adultos, menores, animales heredados, trabajo y embarazo |
| boats | Embarcaciones vacias y remeros en cuatro direcciones |
| buildings | Casas, mercado, granero, muelles y edificios civicos |
| environment | Vegetacion, rocas, cultivos y peces |
| terrain | Materiales usados por el terreno generado |
| effects | Nube de la animacion familiar |
| ui | Fondo del menu |
| audio | Voces de los encuentros familiares |
| licenses | Licencia de los recursos heredados |
| metadata | Procedencia y prompts originales |

## Personajes del video

- [Mujeres: cuatro direcciones y dos pasos](runtime/characters/residents-women-v6.png).
- [Hombres: cuatro direcciones y dos pasos](runtime/characters/residents-men-v6.png).
- [Menores](runtime/characters/residents-children-v6.png).
- [Acciones de mujeres](runtime/characters/residents-work-women-v7.png).
- [Acciones de hombres](runtime/characters/residents-work-men-v7.png).
- [Embarazo](runtime/characters/residents-pregnancy-v7.png).
- [Remeras](runtime/boats/boats-women-v7.png) y [remeros](runtime/boats/boats-men-v7.png).

Las hojas detalladas tienen dos poses por direccion; no son animaciones de ocho pasos. `residents.png` es el atlas pixelado heredado usado para animales. Los personajes detallados se dibujan mediante ResidentSprites y SceneTexture.

`source/geology.png` es una entrada del importador geografico. `archive/` contiene versiones anteriores de terreno, recortes y prototipos; se conservan como historial y no se empaquetan.

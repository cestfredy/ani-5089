# c1-exo8 : Le champ de vision asymétrique

## Réponse

Casque choisi : Meta Quest 3 (à 72 Hz).

| Angle (œil gauche) | Valeur |
|---|:---:|
| gauche | −54,00° |
| droite | 40,00° |
| haut | 43,98° |
| bas | −54,27° |

Source : [HMD Geometry Database, Meta Quest 3 (72Hz)](https://github.com/risa2000/hmdgdb/blob/master/hmd_cfgs/MetaQuest3_Native_R72.md), valeurs « Left eye head FOV » relevées avec l'outil `hmdq` directement dans le runtime Oculus.

Le champ est plus large vers l'extérieur (54° à gauche) que vers le nez (40° à droite), et plus large vers le bas que vers le haut.

Avec un champ symétrique de même surface (environ 47° de chaque côté), on dessinerait des pixels du côté du nez que l'œil ne voit pas à travers la lentille, on couperait une partie du côté extérieur qu'il voit, et la perspective ne correspondrait plus à la lentille, donc les objets paraîtraient déformés ou à la mauvaise profondeur.

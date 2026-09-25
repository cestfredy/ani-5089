# c1-exo2 : Le tableau des budgets

## Réponse

| Étape | Valeur donnée dans le chapitre | Valeur mesurée trouvée | Matériel concerné | Source |
| --- | :---: | --- | --- | --- |
| 1. Les capteurs mesurent le mouvement | 1 à 2 ms | Environ 2 ms entre le mouvement de la tête et la réception par le moteur | Capteur inertiel Oculus Rift DK | Oculus, [Building a Sensor for Low Latency VR](https://www.meta.com/blog/building-a-sensor-for-low-latency-vr/) |
| 2. Le système transmet la mesure | 1 à 3 ms | Jusqu’à 8 ms à 125 Hz et environ 1 ms à 1000 Hz | USB HID, PC | J. Carmack, [Latency Mitigation Strategies](https://danluu.com/latency-mitigation/) |
| 3. L’application décide et dessine | 5 à 11 ms | Valeur mesurée introuvable. Valve donne un budget de 11,11 ms par image à 90 Hz | HTC Vive, GPU | A. Vlachos (Valve), [Advanced VR Rendering](https://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf) |
| 4. Le compositeur assemble | 1 à 2 ms | Valeur mesurée introuvable. Meta indique seulement que le temps du compositeur peut être mesuré avec son outil | Oculus Rift, runtime PC | Meta, [Performance Head-Up Display](https://developers.meta.com/horizon/documentation/native/pc/dg-hud/) |
| 5. L’écran affiche la ligne | 2 à 5 ms | Environ 2 ms pour l’affichage d’une image sur le HTC Vive. Pour le temps de réponse, moins de 1 ms sur OLED et environ 10 ms sur un bon LCD | HTC Vive, OLED/LCD | A. Vlachos (Valve), [Advanced VR Rendering](https://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf) ; J. Carmack, [Latency Mitigation Strategies](https://danluu.com/latency-mitigation/) |
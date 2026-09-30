# Journal de bord du portage

## Entrée 1

Symptôme : `error: failed retrieving file 'libhogweed-4.0-1-x86_64.pkg.tar.zst' from mirror.msys2.org : Connection timed out after 10008 milliseconds` puis `error: failed to commit transaction (download library error)` — `Errors occurred, no packages were upgraded.`

J'ai cru : que l'installation de MSYS2 était cassée et qu'il faudrait tout réinstaller.

C'était : ma connexion, trop lente pour le délai de 10 s de pacman. `pacman -Syu --disable-download-timeout` a suffi.

Temps perdu : 15

## Entrée 2

Symptôme : `'jenga' is not recognized as an internal or external command, operable program or batch file.` juste après `Successfully installed ... jenga-2.8.6`

J'ai cru : que l'installation de Jenga avait échoué.

C'était : `jenga.exe` installé dans `%USERPROFILE%\AppData\Roaming\Python\Python314\Scripts`, dossier absent du PATH (pip l'avait signalé dans un WARNING que je n'avais pas lu).

Temps perdu : 10

## Entrée 3

Symptôme : après `jenga build > sortie-build.txt 2>&1`, `sortie-build.txt` rempli de `[36mÔòöÔòÉÔòÉ...[0m`, illisible. Même résultat avec `jenga build --no-daemon > sortie-build.txt 2>&1`.

J'ai cru : que Jenga produisait une sortie corrompue, puis que le démon de Jenga y était pour quelque chose.

C'était : deux problèmes. PowerShell lit la sortie d'un programme externe avec l'encodage de la console (page de code 850), alors que Jenga écrit en UTF-8 : chaque `╔` ou `✓` devient `Ôòö` ou `Ô£ô`. Et Jenga écrit ses codes couleur ANSI (`[36m`, `[0m`) même vers un fichier. Correction :
`[Console]::OutputEncoding = [Text.Encoding]::UTF8` puis
``jenga build 2>&1 | ForEach-Object { $_ -replace "`e\[[0-9;]*m", "" } | Set-Content -Encoding utf8 sortie-build.txt``

Temps perdu : 10

## Entrée 4

Symptôme : `Compilation Error: main.cpp` avec un cadre d'erreur vide, `Build Failed ... Time: 0.06s`, sur un `main.cpp` qui ne fait que `return 0;`

J'ai cru : que les espaces du chemin (`__stuffs about school`) cassaient la commande de compilation.

C'était : pas le chemin — le même échec se reproduisait dans `C:\tmp\salle`. Le compilateur lui-même échouait (voir entrée 5).

Temps perdu : 15

## Entrée 5

Symptôme : `g++ -std=c++17 -c src\main.cpp -o test.o` → `code: 1`, aucun message, alors que `where.exe g++` trouvait bien `C:\msys64\ucrt64\bin\g++.exe`. `cc1plus.exe` lancé seul : `exit: 0xC0000139`.

J'ai cru : que le terminal avait été ouvert avant l'ajout de MSYS2 au PATH.

C'était : `C:\Program Files\PostgreSQL\17\bin` placé avant `C:\msys64\ucrt64\bin` dans le PATH système. PostgreSQL fournit ses propres `libzstd.dll`, `libwinpthread-1.dll` et `libiconv-2.dll`, plus anciennes. `cc1plus` chargeait la mauvaise DLL et mourait sans rien écrire (0xC0000139 = point d'entrée introuvable dans une DLL). Correction : remonter msys64 au-dessus de PostgreSQL dans le PATH système.

Temps perdu : 60

## Entrée 6

Symptôme : pour exo19, `commandlinetools-win-11076708_latest.zip` refusé par unzip : `End-of-central-directory signature not found`. Le fichier faisait 64 Mo au lieu de 153 Mo.

J'ai cru : que l'archive de Google était abîmée.

C'était : un téléchargement coupé en route, lancé en même temps que ceux du NDK et du JDK. Le relancer seul a suffi.

Temps perdu : 2

## Entrée 7

Symptôme : `jenga build` affiche `Build Successful` et `APK generated`, mais aussi `Debug keystore not found at %USERPROFILE%\.android\debug.keystore - APK ne sera PAS signe ; Android refusera l'install.`

J'ai cru : que le build était bon puisqu'il affichait `Build Successful`.

C'était : un APK non signé, qu'Android refuse d'installer. La clé de debug n'existe qu'après une installation d'Android Studio. Correction : `keytool -genkeypair -keystore ~/.android/debug.keystore -alias androiddebugkey -storepass android -keypass android -keyalg RSA -validity 10000`.

Temps perdu : 3

## Entrée 8

Symptôme : `jenga deploy --run` → `adb.exe: failed to install ... MaSalle.apk: Failure [INSTALL_FAILED_NO_MATCHING_ABIS: Failed to extract native libraries, res=-113]`

J'ai cru : que `jenga deploy` avait pris le mauvais APK, parce qu'il installait la version Release et pas Debug.

C'était : le téléphone (TECNO BF6, Android 12) est en 32 bits : `adb shell getprop ro.product.cpu.abilist` → `armeabi-v7a,armeabi`. Le projet ne construisait que pour `arm64-v8a`. Correction dans `projet.jenga` : `targetarchs([TargetArch.ARM])` et `androidabis(["armeabi-v7a"])`, puis `jenga deploy --platform android --config Debug --run` pour garder les symboles.

Temps perdu : 5

## Entrée 9

Symptôme : `APK installed successfully.`, mais `adb logcat -d -b crash` restait vide.

J'ai cru : que le plantage n'était pas enregistré dans le journal.

C'était : l'application avait été installée mais pas lancée par `jenga deploy --run`. `adb shell monkey -p com.ani5089.masalle -c android.intent.category.LAUNCHER 1` l'a démarrée, et la trace est apparue tout de suite dans le tampon `crash`.

Temps perdu : 3

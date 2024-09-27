# Updater
Progetto in C++ che può essere usato per fare il rollup delle nuove versioni dei tuoi programmi sui PC dei tuoi clienti Windows (che non sanno/o non possono) usare Git client.
Il suo scopo è permettere, tramite la classica GUI, di scaricare una nuova versione dei tuoi Binary *bypassando legalmente* Smartscreen (feature di sicurezza estremamente tossica di Windows) e servire le nuove build dei tuoi programmi agli utenti finali di Windows, senza dover firmare le tue app. Lo script all'interno è presentato con dei commenti per far capire chiaramente come devi impostarlo, per riconoscere la tua repository Github, editali in modo **appropriato** prima di compilare lo script ed inviarlo a Microsoft.

L'utilizzo previso è questo:
1. crea una branch che include le tue ultime release e modifica i campi in accordo all'interno dello script, prima di compilarlo;
2. appena compilato invialo a Microsoft per la notarizzazione. Fatto questo, il tool verrà messo in whitelist e potrà essere usato come GUI Client per far scaricare ai tuoi utenti i tuoi binary file (.exe or .msi) senza doverli firmare, e allo stesso tempo evitando il MOTW e i flag di Smartscreen.

#Security alert: **non rimuovere le funzioni addette a garantire l'interattività con l'utente**. Se il programma non interagisce con l'utente, ed effettua tutto in modo "automatico e silenzioso" potrebbe essere classificato come malware da Microsoft e da altri servizi di sicurezza.

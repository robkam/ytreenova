# YtreeNova Handbuch- und USAGE-Hilfequelle (Deutsch)

Bearbeite diese Datei, um den Referenztext für Handbuch und `docs/USAGE.md` in deutscher Sprache zu pflegen.
Sie ist die lokalisierte Autorenquelle für das Handbuch und bleibt von der kontextuellen `F1`-Hilfe getrennt.

## Topic-Block-Schema

Jeder Themenblock in dieser Datei folgt demselben parserseitigen Vertrag:

1. Der Block beginnt mit einer Überschrift der Ebene 2 in der exakten Form `## topic:<topic-id>`.
2. Direkt danach folgt ein Metadaten-Block mit dem Label `ytnova-help-meta`.
3. Der Metadaten-Block enthält genau diese Schlüssel in genau dieser Reihenfolge:
   * `title:` — der Klartext-Titel des Themas.
   * `contexts:` — kommagetrennte stabile Laufzeit-Kontext- oder Prompt-IDs oder das Literal `none` für reine Link-Themen.
4. Danach enthält der Block diese Abschnitte in dieser Reihenfolge:
   * erforderlich `### Contextual F1`
   * optional `### Explainer links`
   * erforderlich `### Long form`
5. Wenn `### Explainer links` vorhanden ist, benutzt jeder Eintrag Markdown-Linksyntax mit einem `topic:`-Ziel, zum Beispiel `- [Navigation](topic:navigation)`.
6. `### Long form` enthält einen oder mehrere Unterabschnitte der Ebene 4 (`#### ...`). Ihre Reihenfolge bleibt so erhalten, wie sie geschrieben wurde.

Diese Datei folgt der Themeninventur, den `contexts:`-Zuordnungen und den Linkzielen von `etc/help/man.en.md`.
Die englische Datei bleibt die kanonische Quelle für Struktur und Inventar; diese deutsche Datei pflegt die lokalisierte Referenzprosa als Autorenquelle und nicht aus einem generierten Artefakt heraus.

## topic:index

```ytnova-help-meta
title: Inhalt
contexts: none
```

### Contextual F1

Dieses Handbuch ist der ausführliche Referenzpfad für Modi, Prompts, Befehle und Hilfsthemen in ytnova.
Das In-App-`F1`-Popup bleibt der kürzere kontextuelle Pfad für die gerade aktive Oberfläche.

### Long form

#### Zweck

Diese Datei ist die deutsche Autorenquelle für das Handbuch und das generierte `docs/USAGE.md`.
Sie bleibt referenzorientiert; das kontextuelle `F1` erklärt weiter nur die aktuelle Oberfläche.

#### Inhalt

* **Modi und Navigation**: `Directory`, `File`, `Archive-Dir`, `Archive-File`, `Showall`, `Global`, `F7 Preview` und `F8 Split` beschreiben, was die jeweilige Laufzeitoberfläche besitzt.
* **Gemeinsame Regeln**: `Navigation`, `Tagged`, `Shared Commands`, `Command-line Editing`, `Vi Keys`, `F10 Config` und `Theming` sammeln wiederverwendetes Verhalten an genau einer Stelle.
* **Prompt-Referenzen**: `List Jump`, `Copy/Move Targets`, `Filter`, `Compare`, `Output`, `Execute`, `Create Archive` und `Date Change` dokumentieren Syntax, Geltungsbereich und Entscheidungspunkte.
* **Hilfsoberflächen**: `History`, `Volume`, `Applications` und der `F2 Picker` beschreiben die wiederverwendeten Dialoge und Menüs.

## topic:navigation

```ytnova-help-meta
title: Navigation
contexts: none
```

### Contextual F1

Das Hilfepopup benutzt listenartige Navigation.
`Up` und `Down` bewegen, `Enter` oder `Right` folgen, `Left` geht zurück und `Esc` oder `Q` schließt.

### Long form

#### Steuerungstasten-Notation

`C-<chr>` bedeutet: Halte die Control-Taste gedrückt und tippe `<chr>`. `C-f` bedeutet also: Halte Control gedrückt und tippe `f`.

#### Hilfepopup-Tasten

* **Up/Down**: Zwischen auswählbaren Zeilen oder Links bewegen.
* **Page Up/Page Down**: Längere Hilfeseiten scrollen.
* **Home/End**: Zum Anfang oder Ende der aktuellen Hilfeseite springen.
* **Enter/Right**: Das gewählte Hilfethema oder den Link öffnen.
* **Left**: Einen Schritt zurückgehen.
* **Esc/Quit**: Das Popup schließen.

#### Zuständigkeit

Dieses Thema besitzt nur die Navigation innerhalb des Hilfepopups.
Für den Laufzeit-Sprung mit `/` ist `List Jump` zuständig; für normale Verzeichnis- oder Dateiauswahl die lokale Modusseite.

## topic:list-jump

```ytnova-help-meta
title: List Jump
contexts: none
```

### Contextual F1

`/` ist der Namenssprung innerhalb der aktuellen Liste.
Er ist von der Hilfepopup-Navigation getrennt und bleibt immer auf die sichtbare Laufzeitliste beschränkt.

### Long form

#### Sprungmodell

`/` öffnet einen inkrementellen Sprungprompt nur für die gerade sichtbare Liste.
Baum- und Verzeichnisansichten springen durch sichtbare Verzeichnisnamen; dateiorientierte Ansichten springen durch die sichtbaren Dateizeilen dieser Oberfläche.

#### Bestätigen und Abbrechen

* **Text tippen**: Sofort zum besten aktuellen Treffer bewegen.
* **Enter**: Den aktuellen Treffer behalten.
* **Esc**: Den Sprung abbrechen und die ursprüngliche Auswahl wiederherstellen.
* **Geänderter Geltungsbereich**: Filter, Showall, Global, Archive und Split ändern nur, welche sichtbare Liste durchsucht wird, nicht aber die Tasten des Sprungs.

## topic:shared-commands

```ytnova-help-meta
title: Shared Commands
contexts: none
```

### Contextual F1

Diese Funktionstasten behalten ihre grobe Bedeutung über mehrere Modi hinweg.
Oberflächenspezifische Details gehören trotzdem zur jeweiligen Modus- oder Prompt-Seite.

### Long form

#### Gemeinsame Funktionstasten

* **F1**: Kontextuelle Hilfe für die aktive Oberfläche öffnen.
* **F5**: Die aktuelle Ansicht aktualisieren.
* **F6**: Die Detail- oder Statistikdarstellung der aktiven Ansicht ändern.
* **F7**: Vorschau für den aktiven Dateikontext umschalten.
* **F8**: Split-Screen umschalten.
* **F9**: Das Applications-Menü öffnen.
* **F10**: Die Konfigurationsoberfläche öffnen.
* **Esc**: Das aktuelle Overlay, Popup oder den Prompt verlassen.

## topic:tagged

```ytnova-help-meta
title: Tagged
contexts: none
```

### Contextual F1

Markierte Dateien bilden eine Arbeitsmenge für Sammelaktionen, verengte Ansichten, Suchen sowie Archiv- und Exportabläufe.
Tag-basierte Arbeit ist ein zentrales Workflow-Muster von ytnova.

### Long form

#### Grundlagen

Tags sind eine Arbeitsmenge und weder eine zweite Zwischenablage noch eine gespeicherte Suche.
Du baust einen Satz auf, arbeitest damit, verengst ihn und löschst oder invertierst ihn anschließend wieder.

#### Häufige Abläufe

* **Tag/Untag**: Die aktuelle Zeile zur Arbeitsmenge hinzufügen oder daraus entfernen.
* **Invert Tags**: Den Tag-Zustand im sichtbaren Geltungsbereich umdrehen.
* **Filter**: Mit `F`, dann `Tab` zwischen allen Zeilen und nur getaggten Zeilen derselben sichtbaren Liste umschalten.
* **Copy tagged/Move tagged**: Den gesamten getaggten Satz an ein Ziel senden.
* **View tagged**: Die getaggten Dateien nacheinander öffnen.
* **Search tagged**: Nur im getaggten Satz suchen und Nicht-Treffer enttaggen.
* **Archive**: Vorrangig den getaggten Satz archivieren; ohne Tags fällt der Befehl auf die aktuelle Auswahl zurück.

## topic:tagged-viewer

```ytnova-help-meta
title: Markierungsanzeige
contexts: viewer.tagged
```

### Contextual F1

Mit `n` und `p` wechseln Sie Dateien, mit Seitentasten und `Leertaste` blättern Sie in der aktuellen Datei, und mit `/` oder `?` wechseln Sie zwischen Treffern der Markierungssuche. `C-s` durchsucht außerhalb der Anzeige die Markierungsliste.

### Long form

#### Navigationsbereiche

Die interne Markierungsanzeige trennt Datei-, Seiten- und Treffernavigation. Eine externe Markierungsanzeige überlässt Suche und Treffernavigation dem konfigurierten Pager.

## topic:command-line-editing

```ytnova-help-meta
title: Command-line Editing
contexts: none
```

### Contextual F1

Die meisten Prompts teilen sich dieselben Bearbeitungstasten.
Prompt-spezifische Syntax und Bereichsregeln gehören zum jeweiligen Befehlsthema.

### Long form

#### Bearbeitungstasten

* **Left/Right**: Im aktuellen Prompttext bewegen.
* **Home/End**: Zum Anfang oder Ende springen.
* **Backspace/Delete**: Das Zeichen links oder rechts vom Cursor löschen.
* **Enter**: Den aktuellen Wert übernehmen.
* **Esc**: Ohne Übernahme abbrechen.

#### Gemeinsame Helfer

* **Up**: Prompt-Historie öffnen oder durchlaufen, wenn die Oberfläche eine Historie führt.
* **F2**: Browser oder Picker öffnen, wenn der aktuelle Prompt ihn unterstützt.
* **F1**: Syntax- oder Bereichsregeln für genau diesen Prompt anzeigen.

## topic:copy-move-targets

```ytnova-help-meta
title: Copy/Move Targets
contexts: none
```

### Contextual F1

Copy, Move und Pathcopy benutzen zwei explizite Prompts.
Zuerst wählst du Ersatznamen oder Wildcard-Muster, danach das Zielverzeichnis.
Die Trennung bleibt absichtlich bestehen, weil Name/Muster und Ziel zwei verschiedene Entscheidungen sind.
Überschreibkonflikte zeigen Größen- und Zeitinformationen, damit du neuer/älter oder größer/kleiner beurteilen kannst.

### Long form

#### Zielarten

Ein Verzeichnispfad bewahrt die Originalnamen unter einem anderen Ziel.
Ein voller Ersatzname benennt eine einzelne Auswahl ausdrücklich um.
Ein Wildcard-Muster wie `*.bak` oder `copy-*` schreibt die Basenames mehrerer Einträge nach einem Muster um.

#### Gemeinsame Regeln

Tagged Copy/Move benutzt dieselbe Zielsprache wie Einzelelemente.
Pathcopy nutzt denselben Zwei-Prompt-Fluss, bewahrt aber den Pfad relativ zur aktuellen Volume-Wurzel.
Split kann das Verzeichnis des inaktiven Panels als Standardziel vorbelegen; du darfst diesen Wert trotzdem vor dem Start ändern.
Nach Namens- und Zielprompt dürfen nur echte Sicherheitsabfragen folgen, etwa Überschreibkonflikte oder das Erzeugen eines fehlenden Zielverzeichnisses.

## topic:vi-keys

```ytnova-help-meta
title: Vi Keys
contexts: none
```

### Contextual F1

Wenn `VI_KEYS=1`, sind die vi-Navigationstasten in Kleinbuchstaben reserviert.
Kollidierende Befehle wandern dann auf Großbuchstaben oder andere sichere Tasten.

### Long form

#### Navigations-Umschaltung

Mit `VI_KEYS=1` werden `h`, `j`, `k` und `l` zu `Left`, `Down`, `Up` und `Right`.
`C-u` und `C-d` werden zu Seite hoch und Seite runter.

#### Befehlskollisionen

Befehle, die diese Kleinbuchstaben sonst belegen würden, weichen aus.
Beispiele sind `J compare`, `K volume`, `D delete tagged` und `U untag all`, sofern diese Aktionen auf der aktiven Oberfläche vorhanden sind.

## topic:f10

```ytnova-help-meta
title: F10 Config
contexts: none
```

### Contextual F1

F10 besitzt die konfigurationsbezogenen Aktionen, also Profilbearbeitung, Befehlsbearbeitung, Theme-Bearbeitung und Reload.
Es ist die Einrichtungsoberfläche und kein gewöhnlicher Dateibefehl.

### Long form

#### Konfigurationsoberfläche

Benutze `F10`, wenn du dauerhaftes Verhalten ändern willst statt eine einmalige Datei- oder Verzeichnisaktion auszuführen.
Profilwerte, Befehlslabels, Themes und Reload leben hier zusammen.

#### Relevante Dateien

`ytnova.conf` besitzt Profileinstellungen.
`commands.conf` besitzt Benutzerbefehle, Labels und Bindungen.
`themes.conf` besitzt Theme-Auswahl und Theme-Rollenüberschreibungen.

## topic:theming

```ytnova-help-meta
title: Theming
contexts: none
```

### Contextual F1

Themes gestalten semantische UI-Rollen und Dateityp-Paletten.
Theme-Änderungen gehören in die Konfigurationsdateien und nicht in hart codierte Farben pro Bildschirm.

### Long form

#### Theme-Modell

Themes setzen semantische Rollen wie `footer`, `help`, `help_footer`, `help_heading`, `help_topic`, `help_attention`, `help_alert`, `help_keybind`, `help_link`, `help_link_selection`, `selection`, `picker` und `warning`.
So bleiben Footer, Hilfepopup und Auswahlflächen lesbar, ohne dass einzelne Ansichten Spezialfarben im Code erzwingen.

#### Bearbeitungspfad

Öffne die Theme- oder Konfigurationsbearbeitung über `F10`.
Priorisiere zuerst gut lesbare Hochfrequenzflächen: Auswahl, Picker, Footer und Hilfe.

## topic:dir

```ytnova-help-meta
title: Directory Help
contexts: main.dir
```

### Contextual F1

Directory Mode ist die geloggte Baumansicht.
Er besitzt Verzeichnisnavigation, Baumaufbau und verzeichnisbezogene Befehle.

### Long form

#### Verzeichnisnavigation

* **Enter / Right / Left**: `Enter` öffnet das Dateifenster und beendet nötiges Logging. `Right` erweitert zuerst und steigt dann ab. `Left` klappt den aktuellen Knoten zu oder geht zum Elternknoten.
* **Logging-Steuerung**: `+` loggt oder zeigt eine Ebene mehr, `*` erweitert rekursiv, `-` klappt zu und ein zweites `-` auf einem zugeklappten geloggten Knoten setzt ihn wieder auf ungeloggt.
* **Baumzuständigkeit**: Directory Mode besitzt die Form des geloggten Baums. File-Listen, Showall und Global projizieren nur Dateien aus diesem bereits geloggten Baum.

#### Befehlsfamilien

* **Darstellung und Bereich**: `1..9 view` ändert die Paneldarstellung. `0` ist auf Dateisystemen ohne Funktion; `F6` blendet die Statistik ein oder aus. `Filter`, `Showall`, `Global` und `Jump` ändern den projizierten oder sichtbaren Teil.
* **Dateisystemänderungen**: `Attributes`, `Rename`, `Delete`, `Makedir`, `New File` und `Log` ändern Metadaten oder fügen geloggte Wurzeln hinzu.
* **Arbeitsmengensteuerung**: `Tag`, `Untag` und `Invert Tags` definieren die Menge für spätere Sammelbefehle.
* **Transfer und Export**: `Copy`, `MoveDir`, `Output`, `Pipe` und `Archive` arbeiten auf dem gewählten Zweig oder dem getaggten Satz.
* **Oberflächenwechsel**: `Compare`, `Execute`, `Volume`, `Dotfiles` und `Quit` reichen in andere Abläufe oder Sitzungszustände über.

## topic:file

```ytnova-help-meta
title: File Help
contexts: main.file
```

### Contextual F1

File Mode ist die Haupt-Dateilistenansicht.
Er besitzt Dateinavigation, dateibezogene Befehle, Tag-Aktionen und Export-Einstiege.

### Long form

#### Dateinavigation

* **Darstellung**: `1..9 view` bleibt im File Mode und schaltet Name, Attributes, Owner und Times sowie Compact, Größeneinheiten, Mini Preview, File Detail und das Git-Band um. `0` ist auf Dateisystemen ohne Funktion; `F6` blendet die Statistik ein oder aus.
* **Enter**: Zwischen eingebettetem Dateifenster und Vollbild-Dateimodus wechseln, ohne dieselbe Liste zu verlassen.
* **Spalten**: `Left` und `Right` bewegen zwischen sichtbaren Dateispalten; in Einspaltenlayouts blättern sie durch dieselbe Liste.

#### Befehlsfamilien

* **Inspektion**: `View`, `Hex` und `Edit` öffnen die aktuelle Datei im Pager, Hex-Viewer oder Editor.
* **Transfer**: `Copy`, `Move` und `Pathcopy` arbeiten auf der gewählten Datei; `Copy tagged` und `Move tagged` benutzen dieselben Zielregeln für den markierten Satz.
* **Arbeitsmengensteuerung**: `Tag`, `Untag`, `Tag all`, `Untag all` und `Invert Tags` bauen oder leeren den Satz für spätere Sammelbefehle.
* **Listensteuerung**: `Filter`, `Sort`, `Jump` und `Dotfiles` ändern die Projektion der sichtbaren Liste. Der Filterprompt besitzt weiter den Tagged-only-Schalter auf `Tab`.
* **Metadaten und Erzeugung**: `Attributes`, `Rename`, `Delete`, `New File` und `Log` ändern Dateistatus oder laden Inhalte neu ein.
* **Export und Shell-Handoff**: `Output`, `Pipe`, `Execute` und `Archive` exportieren die aktuelle Datei oder den markierten Satz. `Execute` expandiert `{}` für eine Datei; `C-x` wiederholt den Befehl pro markierter Datei.
* **Oberflächenwechsel**: `Compare`, `Search tagged`, `Volume` und `Quit` verzweigen in andere Arbeitsabläufe.

## topic:archive-dir

```ytnova-help-meta
title: Archive Directory Help
contexts: main.archive-dir
```

### Contextual F1

Archive-Dir ist die Baumansicht innerhalb eines geloggten Archivs.
Sie spiegelt Verzeichnisarbeit, soweit das Archivformat dies zulässt.

### Long form

#### Archiv-Verzeichnisnavigation

* **Enter / Left / Right**: Den virtuellen Baum wie im normalen Directory Mode bewegen, aber nur innerhalb des geöffneten Archivs.
* **Wurzelverhalten**: `\` springt aus tieferen Ebenen zur Archivwurzel und verlässt das Archiv ganz, wenn du bereits dort stehst.
* **Archivbereich**: Die Erweiterungszustände sind virtuell; sie spiegeln Archivinhalt und keinen live schreibbaren Dateisystembaum.

#### Archiv-Befehlsfamilien

* **Darstellung und Bereich**: `1..9 view` wählt weiter die Grunddarstellung, nur `9` bleibt im Archiv wirkungslos. `0` ergänzt jede sichtbare Archiv-Dateizeile um Size, Packed und Ratio. Size ist die ursprüngliche Dateigröße, Packed der im Archiv belegte Platz und Ratio der eingesparte Platz in Prozent. Ein Bindestrich bedeutet, dass das Format keine verlässliche gepackte Größe liefert. Die Werte werden beim Laden des Archivs erfasst, sodass `0` nur die Anzeige ändert; erneutes Drücken blendet sie aus. `Filter`, `Showall`, `Global` und `Jump` arbeiten auf der archivgestützten sichtbaren Menge.
* **Archivbewusste Änderungen**: `Delete`, `Rename` und `Makedir` funktionieren nur, wenn Format und Zugriffspfad Rückschreiben zulassen.
* **Arbeitsmenge**: `Tag` und `Untag` gelten für den aktuellen virtuellen Verzeichnisbereich.
* **Transfer und Export**: `Output`, `Pipe`, `Compare`, `Log` und `Volume` benutzen archivgestützte Pfade oder wechseln zu anderen geloggten Wurzeln.
* **Sitzungssteuerung**: `Dotfiles` schaltet versteckte Archiv-Einträge dort um, wo das Format sie zeigt; `Quit` beendet ytnova.

## topic:archive-file

```ytnova-help-meta
title: Archive File Help
contexts: main.archive-file
```

### Contextual F1

Archive-File ist die Dateilistenansicht für archivgestützten Inhalt.
Einige Dateisystembefehle fehlen dort oder verhalten sich archivspezifisch.

### Long form

#### Archiv-Dateinavigation

* **Darstellung**: `1..8` behalten die üblichen Dateibänder; `9` bleibt wirkungslos, weil Archiv-Einträge kein Git-Band besitzen. `0` ergänzt jede sichtbare Archiv-Dateizeile um Size, Packed und Ratio. Size ist die ursprüngliche Dateigröße, Packed der im Archiv belegte Platz und Ratio der eingesparte Platz in Prozent. Ein Bindestrich bedeutet, dass das Format keine verlässliche gepackte Größe liefert. Die Werte werden beim Laden des Archivs erfasst, sodass `0` nur die Anzeige ändert; erneutes Drücken blendet sie aus.
* **Enter**: Zurück in den Archive Directory Mode derselben Archivsitzung.
* **Listensteuerung**: `Jump`, `Filter` und `Sort` wirken weiter auf die sichtbare archivgestützte Dateiliste.

#### Archiv-Befehlsfamilien

* **Inspektion**: `View` und `Hex` öffnen den gewählten Archiv-Eintrag, ohne erst in eine normale Dateisitzung zu wechseln.
* **Transfer**: `Copy`, `Move` und `Pathcopy` benutzen archivbewusste Extraktions- oder Kopierpfade. `Copy tagged` und `Move tagged` gelten entsprechend für den markierten Archivsatz.
* **Arbeitsmenge**: `Tag`, `Untag` und `Invert Tags` verwalten die aktuelle archivgestützte Arbeitsmenge.
* **Mutationsgrenzen**: `Delete` und `Rename` existieren nur dort, wo der Archivpfad Rückschreiben zulässt. `Execute` ist im Archive File Mode nicht verfügbar.
* **Export und Vergleich**: `Output`, `Pipe`, `Compare`, `Search tagged` und `View tagged` bleiben auf die Archivliste begrenzt.
* **Sitzungssteuerung**: `Log`, `Volume`, `Dotfiles` und `Quit` verhalten sich wie im File Mode, können dich aber aus der aktuellen Archivsitzung herausführen.

## topic:filter

```ytnova-help-meta
title: Filter Help
contexts: prompt.filter,prompt.filter-tagged
```

### Contextual F1

Filter wenden Glob-, Ausschluss-, Attribut-, Datums- und Größen-Selektoren auf die aktuelle Dateilistenfamilie an.
Der Prompt startet mit `*`, also allen Dateien.
Mehrere Terme werden mit Kommas gestapelt.

### Long form

#### Syntax

* **Glob-Selektoren**: `*` zeigt alles. `*.c` passt auf ein Muster. `*.c,*.h` stapelt mehrere Einschlussmuster.
* **Ausschlüsse**: Mit `-` vor einem Term, etwa `-*.o`, werden Treffer nach den Einschlüssen wieder abgezogen.
* **Erweiterte Selektoren**: Attributtests wie `:r` oder `:x`, Datumstests wie `>2023-01-01` und Größentests wie `>1M` lassen sich mit Globs mischen.
* **Kombinationen**: `*.c,-*.tmp`, `*.c,*.h,>1M`, `:r,*.sh` und `*.log,>2024-01-01,-debug*` sind gültige zusammengesetzte Filter.
Benutze normale globartige Muster wie `*.c`, kommagetrennte Vereinigungen wie `*.c,*.h`, Ausschlüsse wie `-*.o` und erweiterte Selektoren wie `:r`, `:x`, `>2023-01-01` oder `>1M`.
Wenn deine Shell das Muster schon vor ytnova expandieren würde, setze es an der Shell-Eingabe in Anführungszeichen.

#### Bereich

Der Filter gilt immer für die aktuelle Dateilistenfamilie: normale Dateiliste, Archiv-Dateiliste, Showall oder Global.
`Tab` schaltet innerhalb dieser sichtbaren Familie zwischen allen Zeilen und nur getaggten Zeilen um.
Der Schalter erscheint nur, wenn es im aktuellen Bereich bereits Tags gibt; aktiv zeigt der Prompt `FILTER [tagged only]:`.

## topic:compare

```ytnova-help-meta
title: Compare Help
contexts: none
```

### Contextual F1

Compare deckt Diff-Ansicht, Zielwahl, Bereichswahl, Vergleichsbasis und Ergebnisbehandlung ab.
Die zugehörigen Compare-Themen beschreiben die einzelnen Prompts im Detail.

### Long form

#### Compare-Ablauf

Wähle zuerst das Ziel.
Dann folgt bei Verzeichnisquellen der Vergleichsbereich.
Wenn mehrere Basen verfügbar sind, wählst du danach die Basis.
Zum Schluss bestimmst du, welche Ergebnisklasse auf der Quellseite getaggt werden soll.

#### Compare-Regeln

* Geloggter Baumvergleich benutzt nur geloggten Inhalt und loggt ungeöffnete `+`-Unterverzeichnisse nicht automatisch nach.
* `FILEDIFF` darf `%1` und `%2` verwenden; fehlen diese Platzhalter, hängt ytnova Quell- und Zielpfad an den Hilfsbefehl an.
* Externer Verzeichnis- oder Baumvergleich startet `DIRDIFF` oder `TREEDIFF`, statt Laufzeitergebnisse zu taggen.
* Es gibt keinen separaten Modus nur für getaggte Dateivergleiche.

## topic:compare-target

```ytnova-help-meta
title: Compare Target Help
contexts: prompt.compare-target
```

### Contextual F1

Der Compare-Target-Prompt wählt die andere Datei, das andere Verzeichnis, das andere Panel oder ein externes Viewer-Ziel.
Welche Ziele verfügbar sind, hängt vom aktiven Vergleichsmodus ab.

### Long form

#### Zielregeln

Gib genau einen Pfad ein.
Der gewählte Vergleichsbereich entscheidet dann, ob dieser Pfad als Dateiziel, Verzeichnisziel oder Ziel für einen geloggten Baum interpretiert wird.

## topic:change-date

```ytnova-help-meta
title: Date Change Help
contexts: prompt.change-date
```

### Contextual F1

Der Datums-Prompt akzeptiert `YYYY-MM-DD` sowie optional `HH:MM[:SS]` für Attributänderungen.
`F3` schaltet um, ob der eingegebene Wert Modified, Accessed oder beide Zeitstempel aktualisiert.

### Long form

#### Bereichswahl

Mit `modified` änderst du nur die Änderungszeit.
Mit `accessed` nur die Zugriffszeit.
Mit `both` schreibst du denselben Wert in beide Zeitstempel.

#### Formatregeln

Ohne Zeitanteil behält ytnova Stunde, Minute und Sekunde des aktuellen Werts.
Getaggte Datumsänderungen benutzen denselben Prompt und dieselbe Bereichsumschaltung.

## topic:compare-scope

```ytnova-help-meta
title: Compare Scope Help
contexts: none
```

### Contextual F1

Der Compare-Scope-Prompt wählt Einzelobjekt, getaggten Satz, aktuelles Verzeichnis oder einen größeren Listenbereich.
Welche Optionen genau auftauchen, hängt von der aktiven Oberfläche ab.

### Long form

#### Bereichswahl

`Directory` vergleicht eine Ebene.
`Logged tree` vergleicht den aktuell geloggten rekursiven Baum.
`External viewer` benutzt ein externes Diff-Werkzeug statt Ergebnis-Tags in ytnova.

## topic:compare-basis

```ytnova-help-meta
title: Compare Basis Help
contexts: none
```

### Contextual F1

Der Compare-Basis-Prompt wählt die Kriterien für den aktuellen Vergleichslauf.
Typische Basen sind Name, Größe, Zeit und inhaltlich stärkere Vergleiche.

### Long form

#### Basisauswahl

Wähle die billigste Basis, die deine Frage zuverlässig beantwortet.
`Hash` lohnt sich erst dann, wenn Metadaten allein nicht vertrauenswürdig genug sind.

## topic:compare-results

```ytnova-help-meta
title: Compare Result Help
contexts: none
```

### Contextual F1

Compare-Ergebnisse lassen sich anzeigen, filtern und in eine getaggte Arbeitsmenge für Folgeaktionen umwandeln.
Dieses Thema gehört zur Ergebnisbehandlung.

### Long form

#### Ergebnis-Tagging

Der Compare-Befehl überschreibt keine Dateien.
Er markiert die gewählte Ergebnisklasse auf der aktiven Quellseite, damit du diesen Teil anschließend prüfen, kopieren, bewegen oder archivieren kannst.

## topic:execute-file

```ytnova-help-meta
title: Execute File Help
contexts: prompt.execute-file
```

### Contextual F1

Der File-Execute-Prompt beginnt mit `{}` für den Pfad der gewählten Datei. Gib den Befehl davor und folgende Shell-Syntax danach ein.

### Long form

#### Platzhalterregeln

`{}` steht für einen ausgewählten Dateipfad, zum Beispiel `mv {} /tmp` oder `wc {} > count`.
Beim Tagged-Rerun wird derselbe Befehl einmal pro markierter Datei wiederholt.

## topic:execute-dir

```ytnova-help-meta
title: Execute Directory Help
contexts: prompt.execute-dir
```

### Contextual F1

Der Directory-Execute-Prompt beginnt mit `{}` für den aktuellen Verzeichnispfad. Gib den Befehl davor und folgende Shell-Syntax danach ein.

### Long form

#### Platzhalterregeln

`{}` steht für den aktuellen Verzeichnispfad, zum Beispiel `tar -cf archive.tar {}`.
Der Tagged-Rerun läuft trotzdem über markierte Dateien der aktiven Liste und nicht über irgendeine andere Verzeichnisliste.

## topic:search-tagged

```ytnova-help-meta
title: Search Tagged Help
contexts: prompt.search-tagged
```

### Contextual F1

Search Tagged führt eine Textsuche nur über den markierten Satz aus und entfernt Tags bei Nicht-Treffern.
Es ist eine Verengung einer vorhandenen Arbeitsmenge.

### Long form

#### Regeln für Search Tagged

Baue zuerst einen markierten Satz auf.
Dann suche nur in diesem Satz. Das Ergebnis ist wieder ein markierter Satz, weil Dateien ohne Treffer enttaggt werden.

## topic:create-archive

```ytnova-help-meta
title: Create Archive Help
contexts: prompt.create-archive
```

### Contextual F1

Create Archive baut ein neues Archiv bevorzugt aus dem markierten Satz oder, wenn nichts markiert ist, aus der aktuellen Auswahl.
Welche Archivformate unterstützt sind, hängt vom gewählten Suffix ab.

### Long form

#### Archiv-Erzeugung

Verzeichnisauswahlen werden rekursiv archiviert.
Archive bevorzugen den markierten Satz, weil Tagging der normale Weg ist, einen benutzerdefinierten Batch zusammenzustellen.

## topic:output

```ytnova-help-meta
title: Output Help
contexts: none
```

### Contextual F1

Output exportiert eine oder mehrere Dateien zu einem Ziel, als Raw, Framed oder Page break.
Die zugehörigen Output-Themen beschreiben Format-, Trennzeichen- und Zielprompts.

### Long form

#### Output-Modell

`Output` ist ein Batch-Export und kein Viewer.
Er schreibt die aktuelle Datei oder den markierten Satz als `Raw`, `Framed` oder `Page break` oder sendet denselben Strom an einen Druckerbefehl.

#### Ablauf

Wähle zuerst die Zielklasse: Dateipfad oder Hardcopy.
Bei Dateizielen schaltet `F3` vor der endgültigen Zielangabe zwischen `Raw`, `Framed` und `Page break` um.
Für `Framed` und `Page break` fragt ytnova zuerst das Trennzeichen ab und kehrt dann zum Zielprompt zurück.
Hardcopy fragt nur nach dem Druckerbefehl, weil dort immer roher Ausgabestrom gesendet wird.

## topic:output-format

```ytnova-help-meta
title: Output Format Help
contexts: none
```

### Contextual F1

Output Format bestimmt, wie jede exportierte Datei im Batch eingerahmt wird.
Raw, Framed und Page break dienen verschiedenen Nachbearbeitern oder Lesern.

### Long form

#### Formatwahl

`Raw` eignet sich für weitere Maschinenverarbeitung.
`Framed` und `Page break` sind sinnvoller, wenn Menschen den Exportstapel lesen sollen.

## topic:output-destination

```ytnova-help-meta
title: Output Destination Help
contexts: prompt.output-destination
```

### Contextual F1

Output Destination wählt zuerst Dateiausgabe oder Hardcopy und sammelt dann den endgültigen Zielwert.
Für Dateiausgabe ist `CWD` das aktuelle Arbeitsverzeichnis für nackte Dateinamen.
`F3` schaltet nur im Dateiziel-Prompt zwischen `Raw`, `Framed` und `Page break` um.

### Long form

#### Zielwahl

Dateiausgabe schreibt exportierten Text in einen Pfad.
Hardcopy sendet rohen Exporttext an einen Shell-Druckerbefehl wie `lpr`, `lp` oder `cat > /dev/lp1`.

## topic:output-separator

```ytnova-help-meta
title: Output Separator Help
contexts: prompt.output-separator
```

### Contextual F1

Output Separator erscheint nur, wenn `F3` `Framed` oder `Page break` gewählt hat.
Raw-Ausgabe überspringt diesen Prompt.

### Long form

#### Trennzeichenregeln

Das Trennzeichen wird zwischen Dateien derselben gerahmten oder seitengetrennten Ausgabe wiederverwendet.
Nach der letzten Datei wird es nicht mehr angehängt.

## topic:showall

```ytnova-help-meta
title: Showall Help
contexts: main.showall
```

### Contextual F1

Showall listet alle Dateien des aktuellen geloggten Volumes in einer einzigen aggregierten Dateiliste auf.
Es behält Einzel-Volume-Bereich und entfernt nur die Verzeichnisgrenzen.

### Long form

#### Showall-Verhalten

* **Bereich**: Showall flacht genau ein geloggtes Volume zu einer Dateiliste ab und überschreitet keine andere Volume-Wurzel.
* **Rückweg**: `Esc` kehrt zum Ausgangsverzeichnis zurück; `\` springt zum Besitzerverzeichnis der gewählten Datei innerhalb desselben Volumes.
* **Listensteuerung**: `Sort`, `Filter`, `Jump` und `Dotfiles` arbeiten auf dem aggregierten Showall-Ergebnis. Der Filterprompt behält den Tagged-only-Schalter auf `Tab`.
* **Befehlsfamilie**: Showall benutzt die File-Mode-Befehlsoberfläche mit `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Hex`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Move`, `New File`, `Pipe`, `Rename`, `Sort`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump` und `Dotfiles`; nur der Bereich ist flach und einzelvolumig.

## topic:global

```ytnova-help-meta
title: Global Help
contexts: main.global
```

### Contextual F1

Global listet Dateien aus allen geloggten Volumes in einer einzigen aggregierten Dateiliste.
Es behält Mehr-Volume-Bereich und entfernt nur die Verzeichnisgrenzen.

### Long form

#### Global-Verhalten

* **Bereich**: Global flacht alle geloggten Volumes zu einer Dateiliste ab.
* **Rückweg**: `Esc` kehrt zur vorherigen Verzeichnisoberfläche zurück; `\` springt auch dann zum Besitzerverzeichnis, wenn dieses unter einer anderen Volume-Wurzel liegt.
* **Listensteuerung**: `Filter`, `Jump`, `Dotfiles` und `Sort` wirken auf das aggregierte Global-Ergebnis. Wiederholtes `G` ist wirkungslos, weil du bereits in Global bist.
* **Befehlsfamilie**: Global benutzt dieselbe File-Mode-Befehlsoberfläche wie Showall; der Unterschied ist der volumenübergreifende Bereich und der volumenübergreifende Besitzer-Sprung.

## topic:f7

```ytnova-help-meta
title: F7 Preview Help
contexts: overlay.f7-dir,overlay.f7-file
```

### Contextual F1

F7 Preview legt Vorschau-Steuerung über den darunterliegenden Dateiauswahlkontext.
Die Vorschau besitzt das Scrollen, während die darunterliegende Auswahl weiter die Zieldatei besitzt.

### Long form

#### Vorschau-Navigation

* **Zwei Bereiche bleiben aktiv**: Die Dateiauswahl bewegt sich weiter mit `Up`, `Down`, `PgUp`, `PgDn`, `Home` und `End`; der Vorschaubuffer scrollt mit `Shift-Up/Shift-Down`, `C-p/C-n`, `Shift-PgUp/Shift-PgDn` und `Shift-Home/Shift-End`.
* **Vorschau verlassen**: `F7` oder `Esc` kehren zur suspendierten Directory- oder File-Oberfläche zurück, ohne die Auswahl zu verlieren.
* **Gesperrte Overlays**: `F8` Split und `Tab` zum Panelwechsel sind während aktiver Vorschau deaktiviert.

#### Vorschau-Befehlsfamilien

* **File-Mode-Wiederverwendung**: Preview behält die dateifokussierte Befehlsfamilie mit `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Invert Tags`, `Compare`, `Move`, `New File`, `Rename`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump` und `Dotfiles`.
* **Tagged- und Sammelverhalten**: `C-k` kopiert weiter den markierten Satz, `C-s` startet weiter Search Tagged, ohne die Vorschau zu verlassen.
* **Applications-Handoff**: `F9` öffnet das Applications-Menü aus der Vorschau heraus.

## topic:f8

```ytnova-help-meta
title: F8 Split Help
contexts: none
```

### Contextual F1

Split Mode hält zwei Panels gleichzeitig aktiv, und Laufzeit-`F1` öffnet je nach aktivem Panel die Split-Seite für Directory oder File.
Benutze die lokale Split-Seite für die live sichtbare Footer-Befehlsliste und diese Seite für das gemeinsame Split-Modell.

### Long form

#### Split-Steuerung

* **Panel-Besitz**: Jedes Panel behält Auswahl, Tags, Volume, Ansichtsband und Wiederherstellungszustand. Split ändert nur, welches Panel den nächsten Befehl empfängt.
* **Ziel-Vorgaben**: Copy-, Move- und Compare-Prompts belegen das inaktive Panel als Standardziel oder -vergleich vor, ohne die nachträgliche Bearbeitung zu verbieten.
* **Split verlassen**: `F8` kehrt in den Ein-Panel-Modus zurück. `Tab` wechselt nur das aktive Panel und vermischt keinen Zustand.

## topic:f8-dir

```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-dir
```

### Contextual F1

Die Split-Directory-Seite verbindet die gemeinsamen Split-Regeln mit der aktiven Directory-Footer-Befehlsfamilie.
Sie ist die Laufzeit-`F1`-Seite, wenn der Fokus im Baum-Panel liegt.

### Long form

#### Split-Steuerung

* **Panel-Besitz**: Jedes Panel behält Auswahl, Tags, Volume, Ansichtsband und Wiederherstellungszustand. Split ändert nur, welches Panel den nächsten Befehl empfängt.
* **Ziel-Vorgaben**: Copy-, Move- und Compare-Prompts belegen das inaktive Panel als Standardziel oder -vergleich vor, ohne die nachträgliche Bearbeitung zu verbieten.
* **Split verlassen**: `F8` kehrt in den Ein-Panel-Modus zurück. `Tab` wechselt nur das aktive Panel und vermischt keinen Zustand.

#### Split-Directory-Befehle

Das aktive Split-Directory-Panel benutzt dieselben Befehlsfamilien wie Directory Mode: `1..9 view`, `Attributes`, `Copy`, `Delete`, `Filter`, `Global`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Makedir`, `New File`, `Pipe`, `Rename`, `Showall`, `Tag`, `Untag`, `MoveDir`, `Output`, `Execute`, `Archive`, `Jump` und `Dotfiles`.
Der Unterschied zum Ein-Panel-Directory-Mode liegt nur in der Zielvorbelegung auf das inaktive Panel; `Filter` besitzt weiterhin den Tagged-only-Schalter auf `Tab`.

## topic:f8-file

```ytnova-help-meta
title: F8 Split Help
contexts: overlay.f8-file
```

### Contextual F1

Die Split-File-Seite verbindet die gemeinsamen Split-Regeln mit der aktiven File-Footer-Befehlsfamilie.
Sie ist die Laufzeit-`F1`-Seite, wenn der Fokus im Dateipanel liegt.

### Long form

#### Split-Steuerung

* **Panel-Besitz**: Jedes Panel behält Auswahl, Tags, Volume, Ansichtsband und Wiederherstellungszustand. Split ändert nur, welches Panel den nächsten Befehl empfängt.
* **Ziel-Vorgaben**: Copy-, Move- und Compare-Prompts belegen das inaktive Panel als Standardziel oder -vergleich vor, ohne die nachträgliche Bearbeitung zu verbieten.
* **Split verlassen**: `F8` kehrt in den Ein-Panel-Modus zurück. `Tab` wechselt nur das aktive Panel und vermischt keinen Zustand.

#### Split-File-Befehle

Das aktive Split-File-Panel benutzt dieselben Befehlsfamilien wie File Mode: `1..9 view`, `Attributes`, `Copy`, `Delete`, `Edit`, `Filter`, `Hex`, `Invert Tags`, `Compare`, `Volume`, `Log`, `Move`, `New File`, `Pipe`, `Rename`, `Sort`, `Tag`, `Untag`, `View`, `Output`, `Execute`, `Pathcopy`, `Archive`, `Jump` und `Dotfiles`.
Der Unterschied zum Ein-Panel-File-Mode liegt nur in der Zielvorbelegung auf das inaktive Panel; `Filter` besitzt weiterhin den Tagged-only-Schalter auf `Tab`.

## topic:history-dialog

```ytnova-help-meta
title: History Help
contexts: dialog.history
```

### Contextual F1

Der History-Dialog benutzt frühere Prompt-Einträge wieder und unterstützt Anheften oder Löschen.
Er ist eine gemeinsame Hilfsoberfläche für Prompts mit Historie.

### Long form

#### History-Aktionen

* **Select entry**: Mit `Up` und `Down` durch die aktuelle Historie bewegen.
* **Scroll long entry**: Mit `Left` und `Right` lange Zeilen horizontal verschieben.
* **Pin**: `P` hält einen wichtigen Eintrag oben.
* **Delete**: `D` entfernt den gewählten Eintrag aus dieser Historie.
* **Accept**: `Enter` übernimmt den gewählten Eintrag.
* **Cancel**: `Esc` schließt ohne Übernahme.

## topic:volume-menu

```ytnova-help-meta
title: Volume Help
contexts: dialog.volume-menu
```

### Contextual F1

Das Volume-Menü listet geladene Volumes, lässt dich zu einem wechseln und kann ein Volume freigeben.
Geladene Volumes behalten ihren unabhängigen In-Memory-Zustand, bis sie freigegeben oder neu geladen werden.

### Long form

#### Volume-Aktionen

* **Select volume**: Mit `Up` und `Down` durch die geladene Volume-Liste bewegen.
* **Switch volume**: `Enter` aktiviert das gewählte Volume.
* **Keep state**: Die erneute Auswahl des aktiven Volumes behält dessen In-Memory-Zustand.
* **Release volume**: `D` entlädt das gewählte Volume, solange es nicht das letzte verbleibende ist.
* **Cancel**: `Esc` schließt das Menü.

## topic:applications-menu

```ytnova-help-meta
title: Applications Help
contexts: dialog.applications
```

### Contextual F1

Das Applications-Menü listet konfigurierte Anwendungs-Presets auf.
Mit `Enter` startest du das markierte Preset; ytnova kehrt sofort zurück, während die gestartete Anwendung weiterläuft.
Mit `E` bearbeitest du den Katalog hinter den Presets, mit `Esc` brichst du ab.

### Long form

#### Applications-Aktionen

* **Select preset**: `Up` und `Down` bewegen durch die Preset-Liste.
* **Launcher-Rolle**: `F9` ist der Preset-Starter für wiederkehrende externe Abläufe und etwas anderes als das einmalige `eXecute`.
* **Rückkehrregel**: Nach dem Start kehrt ytnova sofort in die Arbeitsansicht zurück; ein blockierendes `PRESS ENTER` gibt es nicht.
* **Edit presets**: `E` öffnet den dedizierten Applications-Katalog.
* **Selection and working directory**: `{}` setzt die aktuell gewählte Datei oder den Ordner ein; Presets starten im Verzeichnis dieser Auswahl.
* **Prompt text**: `{input}` setzt den zusätzlichen Text ein, den du für das Preset eingegeben hast.
* **Starter presets**: Der gebündelte Katalog beginnt mit `xdg-open`-Startern und enthält kommentierte Beispiele für Werkzeuge wie `mpv` oder lokale Hilfsskripte.
* **Cancel menu**: `Esc` schließt den Chooser ohne Auswahl.

## topic:f2-picker

```ytnova-help-meta
title: F2 Picker Help
contexts: dialog.f2-picker
```

### Contextual F1

Der F2 Picker durchsucht einen Pfad oder ein Preset, das der aktive Prompt unterstützt.
Er ist ein Prompt-Helfer und kein eigenständiger Modus; zusätzlich bietet er Volume-Zyklus, Logging und Dotfile-Umschaltung, ohne den Prompt zu verlassen.

### Long form

#### F2-Picker-Aktionen

* **Move in the tree**: `Up/Down` bewegen die Auswahl, `Left/Right` klappen Teilbäume zu, auf oder steigen hinein.
* **Cycle loaded volumes**: `<` und `>` rotieren durch geloggte Volumes im Picker.
* **Log a new path**: `L` loggt einen neuen Pfad oder ein neues Volume, ohne den Picker zu verlassen.
* **Toggle dotfiles**: `` ` `` übernimmt die Dotfile-Sichtbarkeit der aufrufenden Ansicht auch im Picker.
* **Select or cancel**: `Enter` übernimmt das markierte Verzeichnis, `Esc` bricht den Picker ab.

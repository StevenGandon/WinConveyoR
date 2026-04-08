# .WIZARD — Format de fichier binaire

---

## File Header

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `magic` | 4 | `0x42 0xa4 0x09 0x67` | Magic number identifiant le fichier |
| `endianness` | 1 | `0` = big / `1` = little | Ordre des octets utilisé dans tout le fichier |
| `version` | 2 | hi = major, lo = minor | Octet haut = version majeure, octet bas = version mineure |
| `flags` | 4 | bitmask | bit 0 = align 8 \| bit 1 = align 16 (mutuellement exclusifs) |
| `section_header_off` | 8 | adresse absolue | Offset vers le section header depuis le début du fichier |
| `strndx_off` | 8 | adresse absolue | Offset vers la string table depuis le début du fichier |

---
> **Alignement**
---

## Section Header

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `header_size` | 8 | entier | Taille totale du section header |
| `section_count` | 8 | entier | Nombre de sections dans le fichier |

### Entry × section_count — 28 octets chacune

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `name_ref` | 4 | offset strndx | Référence vers le nom de la section dans la string table |
| `type` | 4 | enum `0x00`–`0x05` | Type de la section (voir [Types & Flags](#types--flags)) |
| `flags` | 4 | bitmask | Plateforme cible : bit 0 = windows, bit 1 = posix |
| `size` | 8 | entier | Taille en octets du contenu de la section |
| `offset` | 8 | adresse absolue | Offset vers le contenu de la section depuis le début du fichier |

---
> **Alignement**
---

## Sections × N

### Section — metadata (type `0x05`)

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `données` | variable | — | Métadonnées du paquet (nom, version, description, dépendances, architecture, machine) |

### Section — build (type `0x04`)

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `instructions` | variable | séquence | Script de compilation, exécuté avant installation |

### Section — install (type `0x01`)

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `instructions` | variable | séquence | Script d'installation des fichiers du paquet |

### Section — uninstall (type `0x02`)

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `instructions` | variable | séquence | Script de suppression des fichiers installés |

### Section — purge (type `0x03`)

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `instructions` | variable | séquence | Script de suppression complète, y compris les données utilisateur |

### Instruction — répétée dans chaque section de code

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `opcode` | 2 | entier | Code de l'opération (défini dans `typedef_x64.xml` / `instructions_v0.xml`) |
| arg scalaire | N (fixe) | entier | Taille fixe définie par le typedef associé à l'opcode |
| arg string | 4 | offset strndx | Référence vers une chaîne dans la string table |
| arg tableau | 4 + N×M | count + items | Nombre d'éléments (4 octets) suivi des éléments de taille M chacun |

---
> **Alignement**
---

## String Table

Adressée par `strndx_off`. Entrée répétée pour chaque chaîne unique, référencée par offset depuis le début de la table.

| Champ | Taille | Valeur / Encodage | Description |
|---|---|---|---|
| `len` | 4 | entier | Longueur en octets de la chaîne suivante |
| `string bytes` | len | variable | Contenu brut de la chaîne (noms de sections, valeurs d'arguments) |

---

## Types & Flags

### Section types

| Constante | Valeur | Description |
|---|---|---|
| `TYPE_GENERIC_SECTION` | `0x00` | Section générique |
| `TYPE_SECTION_INSTALL` | `0x01` | Script d'installation |
| `TYPE_SECTION_UNINSTALL` | `0x02` | Script de désinstallation |
| `TYPE_SECTION_PURGE` | `0x03` | Suppression complète |
| `TYPE_SECTION_BUILD` | `0x04` | Script de compilation |
| `TYPE_SECTION_METADATA` | `0x05` | Métadonnées du paquet |

### Section flags

| Constante | Bit | Description |
|---|---|---|
| `FLAGS_DEFAULT` | — | Aucune plateforme spécifique (`0x00`) |
| `FLAGS_WINDOWS` | bit 0 | Section destinée à Windows |
| `FLAGS_POSIX` | bit 1 | Section destinée à POSIX / Linux / macOS |

---

## Arguments d'instruction

| Type | Classe Python | Taille | Description |
|---|---|---|---|
| Scalaire | `WizardArgument` | N (fixe) | Entier de taille fixe définie par le typedef. Encodé selon l'endianness du fichier. |
| String ref | `WizardArgument` (str) | 4 | Offset dans la string table. La chaîne est enregistrée par `add_strndx()` lors de la sérialisation. |
| Tableau | `WizardArgumentArray` | 4 + N×M | count (4 octets) suivi de N éléments de taille M. Les éléments peuvent être des scalaires ou des string refs. |

> `_ARR_COUNT_SIZE = 4` — `get_size()` d'un tableau = `4 + nb_items × taille_item`

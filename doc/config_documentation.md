# Configuration d'un paquet — `config.yaml`

Un fichier `config.yaml` décrit comment compiler, installer, désinstaller et purger un paquet.
Il est analysé par `YAMLConfigReader` et compilé en binaire `.WIZARD`.

---

## Structure globale

Tout fichier de configuration doit commencer par la clé racine `wizard`.

```yaml
wizard:
  version: 1
  metadata: ...
  build: ...
  install: ...
  uninstall: ...
  purge: ...
```

| Clé | Obligatoire | Description |
|---|---|---|
| `version` | Non (défaut `1`) | Version du format de configuration. Doit être un entier, actuellement entre `1` et `1`. |
| `metadata` | Non | Identité du paquet et plateforme cible. |
| `build` | Non | Étapes de compilation depuis les sources. |
| `install` | Non | Étapes d'installation sur le système. |
| `uninstall` | Non | Étapes de suppression des fichiers installés. |
| `purge` | Non | Étapes de suppression complète, y compris les données utilisateur. |

---

## `metadata`

Décrit l'identité du paquet. Tous les champs sont optionnels et ont des valeurs par défaut.

```yaml
metadata:
  name: mon_paquet
  description: "Une courte description"
  version: "1.0.0"
  deps:
    - python
    - make
  machine: posix
  architecture: any
```

| Champ | Type | Défaut | Description |
|---|---|---|---|
| `name` | chaîne | `new_package` | Nom du paquet. Utilisé pour nommer les fichiers de sortie. |
| `description` | chaîne | `new package.` | Description courte lisible par un humain. |
| `version` | chaîne | `1.0.0` | Version du paquet. |
| `deps` | liste de chaînes | `[]` | Noms des paquets dont celui-ci dépend. |
| `machine` | chaîne | `any` | Famille d'OS cible. Valeurs acceptées : `posix`, `linux`, `windows`, `macos`, `any`. |
| `architecture` | chaîne | `any` | Architecture CPU cible (ex. `x64`, `arm64`, `any`). |

---

## Jobs — `build`, `install`, `uninstall`, `purge`

Chaque job est optionnel. S'il est présent, il contient une liste `steps` et optionnellement un bloc `requires`.

```yaml
build:
  requires:
    tools: [make, gcc]
  steps:
    - ...
```

### `requires` (build uniquement)

| Champ | Type | Défaut | Description |
|---|---|---|---|
| `tools` | liste de chaînes | `[]` | Outils devant être disponibles pour exécuter ce job (informatif). |

### `steps`

Une liste d'objets étape. Chaque étape est un dictionnaire à clé unique : la clé est le nom de l'instruction, la valeur contient ses arguments.

```yaml
steps:
  - mkdir:
      path: build
  - run:
      tool: make
      args:
        - re
```

> Chaque étape peut déclarer un bloc `when` pour restreindre son exécution à une plateforme spécifique (voir ci-dessous).

---

## `when` — condition de plateforme

N'importe quelle étape peut déclarer un bloc `when`. Si omis, l'étape s'exécute sur toutes les plateformes (`any`).

```yaml
- copy:
    when:
      os: posix
    from: mon_binaire
    to: /usr/local/bin/mon_binaire
```

| Champ | Valeurs | Description |
|---|---|---|
| `os` | `any`, `posix`, `linux`, `windows`, `macos` | Restreint cette étape à la famille d'OS indiquée. |

Les étapes partageant la même valeur `os` sont compilées dans la même section `.WIZARD`.
Les étapes avec `os: any` sont ajoutées à toutes les sections de plateforme, ou dans une section `any` dédiée si aucune étape spécifique à une plateforme n'existe.

---

## Instructions disponibles

### `mkdir` — créer un répertoire

Crée un répertoire au chemin indiqué. Ne fait rien s'il existe déjà.

```yaml
- mkdir:
    path: build
```

| Champ | Obligatoire | Description |
|---|---|---|
| `path` | Oui | Chemin du répertoire à créer. |

---

### `run` — exécuter un outil

Exécute un programme externe avec des arguments optionnels.

```yaml
- run:
    tool: make
    args:
      - re
```

| Champ | Obligatoire | Description |
|---|---|---|
| `tool` | Oui | Nom ou chemin de l'exécutable (cherché dans le `PATH`). |
| `args` | Non | Liste d'arguments passés à l'outil. |

---

### `copy` — copier un fichier

Copie un fichier d'un chemin vers un autre.

```yaml
- copy:
    from: mon_binaire
    to: /usr/local/bin/mon_binaire
```

| Champ | Obligatoire | Description |
|---|---|---|
| `from` | Oui | Chemin source. |
| `to` | Oui | Chemin de destination. |

---

### `chmod` — modifier les permissions d'un fichier

Définit les permissions d'un fichier. La valeur est un entier octal.

```yaml
- chmod:
    path: /usr/local/bin/mon_binaire
    mode: 0755
```

| Champ | Obligatoire | Description |
|---|---|---|
| `path` | Oui | Chemin du fichier. |
| `mode` | Oui | Bits de permission en octal (ex. `0755`, `0644`). |

---

### `remove` — supprimer un fichier

Supprime un fichier unique.

```yaml
- remove:
    path: /usr/local/bin/mon_binaire
```

| Champ | Obligatoire | Description |
|---|---|---|
| `path` | Oui | Chemin du fichier à supprimer. Supporte l'expansion `${VAR}`. |

---

### `remove_tree` — supprimer un répertoire récursivement

Supprime un répertoire et tout son contenu.

```yaml
- remove_tree:
    path: /var/mon_paquet
```

| Champ | Obligatoire | Description |
|---|---|---|
| `path` | Oui | Chemin du répertoire à supprimer récursivement. Supporte l'expansion `${VAR}`. |

---

## Expansion de variables d'environnement

Les chaînes de chemin supportent la substitution `${VAR}`. Les variables suivantes sont disponibles à l'exécution :

| Variable | Description |
|---|---|
| `${MACHINE_BIN}` | Répertoire des binaires de la plateforme (ex. `/usr/local/bin` sur POSIX). |
| `${PREFIX}` | Préfixe d'installation (ex. `/usr/local`). |

---

## Exemple complet

```yaml
wizard:
  version: 1

  metadata:
    name: test_package
    description: "A test package"
    version: "1.0.0"
    deps:
      - python
    machine: posix
    architecture: any

  build:
    requires:
      tools: [make, gcc]
    steps:
      - mkdir:
          path: build
      - run:
          tool: make
          args:
            - re
      - run:
          tool: make
          args:
            - clean

  install:
    steps:
      - copy:
          when:
            os: posix
          from: test_package
          to: ${MACHINE_BIN}/test_package
      - chmod:
          when:
            os: posix
          path: ${MACHINE_BIN}/test_package
          mode: 0755

  uninstall:
    steps:
      - remove:
          path: "${MACHINE_BIN}/test_package"

  purge:
    steps:
      - remove_tree:
          path: "${PREFIX}/var/test_package"
```

---

## Notes et contraintes

- Un objet étape doit avoir **exactement une** clé d'instruction au niveau supérieur. Plusieurs clés au même niveau provoquent une erreur de parsing.
- `version` doit être un entier. Un champ `version` absent bascule sur la dernière version supportée avec un avertissement.
- Les noms d'instructions inconnus (absents du XML de l'ensemble d'instructions chargé) provoquent une `ValueError` à la compilation.
- Les valeurs de `when.os` hors de `[posix, linux, windows, macos, any]` sont acceptées par le parser mais peuvent ne produire aucune section de plateforme correspondante.

# WinConveyoR — Choix Technologiques

> Document technique détaillant et justifiant les choix de technologies pour chaque couche du projet WinConveyoR.

---

## Table des matières

1. [Vue d'ensemble de l'architecture](#vue-densemble-de-larchitecture)
2. [Couche noyau — libwconr (C)](#couche-noyau--libwconr-c)
3. [Format binaire — .WIZARD](#format-binaire--wizard)
4. [Protocole réseau — WCR (TCP)](#protocole-réseau--wcr-tcp)
5. [Cryptographie — OpenSSL](#cryptographie--openssl)
6. [CLI — Python + ctypes](#cli--python--ctypes)
7. [API REST — FastAPI](#api-rest--fastapi)
8. [Interface graphique — Electron + React + TypeScript](#interface-graphique--electron--react--typescript)
9. [Composants UI — shadcn/ui](#composants-ui--shadcnui)
10. [Base de données — PostgreSQL + Prisma](#base-de-données--postgresql--prisma)
11. [Infrastructure miroir — Docker](#infrastructure-miroir--docker)
12. [Outillage — Git, WSL, Linux](#outillage--git-wsl-linux)
13. [Synthèse comparative](#synthèse-comparative)

---

## Vue d'ensemble de l'architecture

WinConveyoR est un gestionnaire de paquets pour Windows composé de plusieurs couches complémentaires :

```
┌─────────────────────────────────────────────┐
│           GUI (Electron + React + TS)       │
├─────────────────────────────────────────────┤
│              API REST (FastAPI)             │
├─────────────────────────────────────────────┤
│           CLI (Python + ctypes)             │
├─────────────────────────────────────────────┤
│         Noyau natif — libwconr (C)          │
│   ┌───────────┬──────────┬────────────┐     │
│   │   mmap    │ OpenSSL  │ WCR (TCP)  │     │
│   └───────────┴──────────┴────────────┘     │
├─────────────────────────────────────────────┤
│     Infrastructure miroir (Docker)          │
├─────────────────────────────────────────────┤
│        Format de paquets (.WIZARD)          │
└─────────────────────────────────────────────┘
```

Chaque couche a été choisie pour répondre à des contraintes précises de performance, de maintenabilité et de portabilité.

---

## Couche noyau — libwconr (C)

### Pourquoi le C ?

Le cœur de WinConveyoR manipule des fichiers binaires, des sockets réseau bruts et des zones de mémoire mappées. Le C s'impose ici pour plusieurs raisons :

- **Contrôle mémoire total.** Le parsing du format `.WIZARD` repose sur `mmap`, ce qui nécessite un accès direct à la mémoire virtuelle du système d'exploitation. Le C permet de travailler avec des pointeurs, des structures alignées et des casts sans couche d'abstraction.
- **Performance prévisible.** Un gestionnaire de paquets doit pouvoir scanner, vérifier et extraire des centaines de paquets rapidement. Le C ne génère aucun overhead de runtime (pas de garbage collector, pas de VM), ce qui garantit des temps d'exécution constants et prévisibles.
- **Interopérabilité maximale.** Une bibliothèque C peut être appelée depuis pratiquement n'importe quel langage via FFI (Foreign Function Interface). C'est ce qui permet à la CLI Python d'utiliser `ctypes` pour appeler directement les fonctions de `libwconr`, et potentiellement à d'autres frontends de le faire à l'avenir.
- **Précédent industriel.** Les gestionnaires de paquets les plus performants (apt/dpkg, pacman, rpm) sont écrits en C. Ce choix s'inscrit dans une tradition éprouvée pour ce type d'outil système.

### Pourquoi pas Rust ?

Rust aurait offert des garanties de sécurité mémoire au compile-time, mais au prix d'une courbe d'apprentissage significative et d'un écosystème FFI moins mature que celui du C. Le C offre une compatibilité ABI native avec les systèmes cibles (Windows, Linux) et une intégration directe avec OpenSSL sans wrapper supplémentaire. Le choix du C est aussi un choix pédagogique cohérent avec la formation Epitech, qui met l'accent sur la maîtrise du bas niveau.

### Pourquoi `mmap` ?

Le memory-mapping (`mmap`) permet de lire les fichiers `.WIZARD` sans les charger intégralement en mémoire. Le noyau du système d'exploitation gère le chargement paresseux des pages, ce qui offre :

- **Accès aléatoire efficace** aux différentes sections du fichier binaire (header, metadata, payload) sans seek/read multiples.
- **Empreinte mémoire réduite** pour les paquets volumineux : seules les pages réellement accédées sont chargées en RAM.
- **Simplicité du code de parsing** : les structures C peuvent être directement superposées sur la zone mappée via des pointeurs typés.

---

## Format binaire — .WIZARD

### Pourquoi un format binaire custom ?

Les gestionnaires de paquets existants sur Windows (Chocolatey, Scoop, winget) utilisent des formats textuels (YAML, JSON, NuGet/XML) ou des archives standards (ZIP, NUPKG). WinConveyoR fait un choix différent avec `.WIZARD`, un format binaire structuré, pour plusieurs raisons :

- **Parsing en O(1).** Un header à taille fixe permet d'accéder aux métadonnées du paquet (nom, version, dépendances, checksums) sans parser l'intégralité du fichier. Avec un format texte, il faut lire et parser séquentiellement.
- **Intégrité intégrée.** Le format embarque directement les hash SHA-256 dans ses sections, éliminant le besoin de fichiers de signature externes.
- **Cohérence avec `mmap`.** Un format binaire à structure fixe est idéal pour le memory-mapping : les offsets sont connus à l'avance, les structures C se superposent directement.

### Pourquoi s'inspirer d'ELF ?

Le format ELF (Executable and Linkable Format) est un standard robuste et éprouvé depuis plus de 30 ans pour structurer des fichiers binaires. L'architecture de `.WIZARD` reprend ses principes fondamentaux :

- **Magic number** en début de fichier pour l'identification rapide.
- **Table de sections** avec offsets et tailles, permettant un accès direct à n'importe quelle section.
- **Extensibilité** : de nouvelles sections peuvent être ajoutées sans casser la rétrocompatibilité, car le parser les ignore si elles ne sont pas reconnues.

Ce modèle est préféré à une structure ad hoc parce qu'il est documenté, compris par les développeurs système, et qu'il a fait ses preuves en production à très grande échelle.

---

## Protocole réseau — WCR (TCP)

### Pourquoi un protocole custom sur TCP ?

Le protocole WCR (WinConveyoR Remote) est un protocole binaire applicatif conçu pour la synchronisation des paquets entre le client et les serveurs miroir.

- **Optimisation bande passante.** Contrairement à HTTP/REST qui transporte des en-têtes textuels volumineux, WCR utilise des frames binaires compactes. Pour la synchronisation d'index (opération fréquente), cela réduit significativement le volume de données échangées.
- **Opérations atomiques.** WCR définit des opcodes spécifiques (SYNC, FETCH, VERIFY) qui correspondent exactement aux opérations du gestionnaire de paquets. Chaque requête-réponse est une unité logique complète, sans le overhead d'un protocole générique.
- **Reprise sur erreur.** Le protocole intègre nativement des mécanismes de reprise de transfert partiel, ce qui est critique pour le téléchargement de paquets volumineux sur des connexions instables.

### Pourquoi TCP et pas UDP/QUIC ?

TCP offre la fiabilité de livraison et l'ordonnancement des paquets dont WCR a besoin sans avoir à les réimplémenter. QUIC aurait pu offrir du multiplexage et une latence réduite, mais au prix d'une complexité d'implémentation disproportionnée pour un premier protocole. TCP reste le choix le plus pragmatique et le plus débuggable.

---

## Cryptographie — OpenSSL

### Pourquoi OpenSSL ?

La vérification d'intégrité des paquets (SHA-256) et la communication sécurisée avec les miroirs nécessitent une bibliothèque cryptographique fiable :

- **Standard industriel.** OpenSSL est la bibliothèque cryptographique la plus déployée au monde. Ses implémentations de SHA-256 sont auditées, optimisées (accélération matérielle via AES-NI/SHA extensions) et constamment maintenues.
- **API C native.** Étant écrite en C, OpenSSL s'intègre directement dans `libwconr` sans binding, sans FFI, sans overhead. Les appels sont des appels de fonctions standard.
- **Disponibilité universelle.** OpenSSL est disponible sur toutes les plateformes cibles (Windows via vcpkg/MSYS2, Linux via les package managers système).

### Pourquoi pas libsodium ou une implémentation maison ?

Libsodium offre une API plus simple et des garanties de sécurité par défaut, mais son écosystème est plus réduit et elle est moins présente sur les environnements Windows natifs. Une implémentation cryptographique maison est exclue par principe : les erreurs d'implémentation cryptographique sont la première source de vulnérabilités dans les logiciels de distribution de paquets.

---

## CLI — Python + ctypes

### Pourquoi Python pour la CLI ?

La CLI est le premier point d'interaction utilisateur avec WinConveyoR. Python est choisi comme langage de scripting pour cette couche :

- **Prototypage rapide.** Python permet d'itérer rapidement sur l'UX en ligne de commande (parsing d'arguments, formatage de sortie, gestion des erreurs) sans recompilation.
- **Écosystème riche.** Les bibliothèques comme `argparse`, `rich` (affichage terminal), `pathlib` (manipulation de chemins cross-platform) accélèrent le développement.
- **Accessibilité pour les contributeurs.** Python est un langage accessible qui abaisse la barrière d'entrée pour les contributions à la CLI, tout en gardant la performance critique dans la couche C.

### Pourquoi ctypes ?

`ctypes` est le module standard de Python pour appeler des bibliothèques C partagées (`.so` / `.dll`). Il est préféré aux alternatives pour plusieurs raisons :

- **Zéro dépendance externe.** Contrairement à `cffi` ou `cython`, `ctypes` fait partie de la bibliothèque standard Python. Aucune installation supplémentaire n'est requise.
- **Binding déclaratif.** Les signatures de fonctions C sont déclarées en Python, ce qui sert de documentation vivante de l'API de `libwconr`.
- **Pas de compilation.** L'utilisation de `ctypes` ne nécessite pas de phase de compilation côté Python, simplifiant la chaîne de build et le packaging.

---

## API REST — FastAPI

### Pourquoi FastAPI ?

L'API REST sert de pont entre le noyau C/Python et l'interface graphique Electron. FastAPI est choisi pour cette couche intermédiaire :

- **Performance async native.** FastAPI est basé sur Starlette et ASGI, offrant un modèle asynchrone natif. Les opérations longues (téléchargement de paquets, synchronisation) ne bloquent pas les autres requêtes.
- **Typage automatique.** L'intégration de Pydantic permet de valider et documenter les payloads de requête/réponse via des annotations de type Python. L'API est auto-documentée via Swagger UI et ReDoc sans effort supplémentaire.
- **Cohérence avec la CLI.** La CLI et l'API partagent le même runtime Python et les mêmes bindings `ctypes` vers `libwconr`. Cela évite la duplication de logique et garantit la cohérence entre les deux interfaces.
- **Déploiement léger.** FastAPI avec Uvicorn se lance en une commande et consomme peu de ressources, ce qui est adapté à une utilisation locale (l'API tourne sur la machine de l'utilisateur).

### Pourquoi pas Flask ou Django ?

Flask est synchrone par défaut, ce qui le rend moins adapté aux opérations I/O-bound (réseau, filesystem) du gestionnaire de paquets. Django est surdimensionné pour une API qui ne nécessite ni ORM, ni système de templates, ni admin. FastAPI occupe le créneau idéal : léger, typé, performant.

---

## Interface graphique — Electron + React + TypeScript

### Pourquoi Electron ?

WinConveyoR est un outil pour Windows qui nécessite une interface graphique desktop riche. Electron est choisi comme runtime :

- **Accès au système.** Electron combine Chromium (rendu web) et Node.js (accès système), permettant à l'UI de lire le filesystem, exécuter des processus, et communiquer avec l'API locale — tout ce qu'un navigateur web ne permet pas.
- **Écosystème de distribution.** `electron-builder` et `electron-forge` fournissent des pipelines de build, d'auto-update et d'installeurs (MSI, NSIS) matures pour Windows.
- **Cross-platform potentiel.** Bien que WinConveyoR cible Windows, l'utilisation d'Electron ne ferme pas la porte à un portage futur sur macOS ou Linux avec un effort minimal.

### Pourquoi pas Tauri ?

Tauri offre des binaires plus légers en utilisant le WebView système au lieu d'embarquer Chromium. Cependant, le WebView de Windows (WebView2/Edge) présente des inconsistances de rendu selon les versions de Windows, et l'écosystème de plugins Tauri est moins mature qu'Electron pour les besoins de WinConveyoR (accès shell, gestion de processus, auto-update). Le surcoût en taille de bundle d'Electron est un compromis acceptable pour la fiabilité du rendu et la maturité de l'écosystème.

### Pourquoi React ?

- **Composabilité.** L'interface de WinConveyoR est composée de vues complexes (liste de paquets, détails, recherche, paramètres, logs). Le modèle de composants React permet de découper chaque vue en unités autonomes et réutilisables.
- **Écosystème dominant.** React dispose du plus grand écosystème de composants, de hooks, et de bibliothèques UI dans le monde JavaScript. Ce choix maximise les options disponibles pour chaque problème d'interface.
- **React Native.** La compétence React est directement transférable à React Native pour un éventuel companion mobile.

### Pourquoi TypeScript ?

- **Sécurité au compile-time.** TypeScript détecte les erreurs de type avant l'exécution, ce qui est critique dans une application de bureau où les crashs dégradent directement l'expérience utilisateur.
- **Autocomplétion et refactoring.** Le typage statique améliore la productivité du développement dans un IDE (VS Code) et rend le code plus navigable pour les contributeurs.
- **Contrat d'interface explicite.** Les types TypeScript documentent la forme des données échangées entre les composants React et l'API FastAPI, servant de source de vérité pour le schéma des données côté frontend.

---

## Composants UI — shadcn/ui

### Pourquoi shadcn/ui ?

L'interface graphique de WinConveyoR utilise shadcn/ui comme base de composants :

- **Ownership du code.** Contrairement aux bibliothèques classiques (MUI, Ant Design, Chakra), shadcn/ui copie les composants directement dans le projet via CLI. Les fichiers appartiennent au développeur, pas à un `node_module`. Cela permet une personnalisation profonde sans fork ni override CSS fragile.
- **Radix UI en fondation.** Les composants shadcn/ui sont construits sur Radix UI, qui fournit des primitives headless accessibles (ARIA, focus management, keyboard navigation) sans imposer de style visuel. L'accessibilité est gérée par défaut.
- **Tailwind CSS natif.** shadcn/ui utilise Tailwind CSS pour le styling, ce qui s'intègre naturellement dans le workflow React/TypeScript et permet une personnalisation via des CSS variables et des classes utilitaires.
- **Personnalisation thématique poussée.** WinConveyoR utilise un thème custom inspiré de VS Code dark mode avec une palette chaude (`#FFF7EE` fond clair, `#E8913A` accent orange, near-black pour le dark mode), des coins nets (sharp corners), et des layouts denses. shadcn/ui rend cette personnalisation triviale via la surcharge de ses CSS variables sans toucher au code des composants.

### Pourquoi pas MUI ou Ant Design ?

MUI et Ant Design imposent une identité visuelle forte (Material Design, Ant Design System) qui nécessite un effort significatif pour s'en éloigner. Leur modèle de theming par override CSS crée des conflits de spécificité difficiles à maintenir. shadcn/ui, en donnant la propriété du code source des composants, élimine cette friction.

---

## Base de données — PostgreSQL + Prisma

### Pourquoi PostgreSQL ?

La gestion des métadonnées de paquets, des utilisateurs des miroirs et de l'état de synchronisation nécessite une base de données relationnelle :

- **Fiabilité ACID.** PostgreSQL garantit l'intégrité transactionnelle, ce qui est non-négociable pour un registre de paquets où une corruption de métadonnées peut entraîner l'installation de versions incorrectes ou compromises.
- **Performances.** Les index B-tree et GIN de PostgreSQL permettent des recherches rapides dans les catalogues de paquets, y compris des recherches full-text sur les descriptions.
- **JSON natif.** Le type `jsonb` permet de stocker des métadonnées de paquets à structure variable (tags, compatibilité système, notes de version) sans sacrifier les capacités de requête SQL.
- **Écosystème et communauté.** PostgreSQL est open-source, massivement documenté, et supporté par tous les hébergeurs cloud.

### Pourquoi Prisma ?

- **Type-safety end-to-end.** Prisma génère un client TypeScript typé à partir du schéma de la base de données. Les requêtes sont vérifiées au compile-time, éliminant les erreurs de nom de colonne ou de type.
- **Migrations déclaratives.** Le schéma Prisma (`schema.prisma`) sert de source de vérité pour la structure de la base. Les migrations sont générées automatiquement par diff, réduisant le risque d'erreurs manuelles.
- **DX (Developer Experience).** L'autocomplétion des requêtes dans l'IDE accélère le développement et rend le code plus lisible que du SQL brut ou un query builder.

---

## Infrastructure miroir — Docker

### Pourquoi Docker ?

Les serveurs miroir qui hébergent et distribuent les paquets `.WIZARD` sont conteneurisés avec Docker :

- **Reproductibilité.** Un `Dockerfile` garantit que chaque miroir est configuré de manière identique, quel que soit l'hôte. Cela élimine les problèmes de "ça marche sur ma machine" et simplifie le déploiement de nouveaux miroirs par la communauté.
- **Isolation.** Chaque miroir tourne dans son conteneur avec ses propres dépendances (FastAPI, OpenSSL, volumes de stockage), sans interférer avec les autres services de l'hôte.
- **Scalabilité horizontale.** Docker Compose (ou Kubernetes à plus grande échelle) permet de déployer plusieurs instances de miroir derrière un load balancer, avec des volumes partagés pour le stockage de paquets.
- **CI/CD natif.** Les images Docker s'intègrent directement dans les pipelines GitHub Actions pour le build, le test et le déploiement automatisé des miroirs.

### Pourquoi pas des VMs ou du bare-metal ?

Les machines virtuelles ajoutent un overhead significatif (OS complet, boot time) inutile pour des services stateless comme les miroirs. Le bare-metal offre les meilleures performances mais sacrifie la portabilité et la reproductibilité. Docker offre le meilleur compromis pour un projet open-source où les miroirs doivent être déployables par n'importe qui.

---

## Outillage — Git, WSL, Linux

### Git

Git est le standard de facto pour le versioning de code. Son utilisation dans WinConveyoR suit des conventions strictes :

- Commits atomiques entre chaque étape de développement.
- Branches feature pour l'isolation du travail en cours.
- Tags sémantiques pour le versioning des releases.

### WSL (Windows Subsystem for Linux)

Le développement de WinConveyoR se fait sous WSL pour plusieurs raisons :

- **Toolchain C standard.** GCC, Make, et les outils de build Unix sont natifs sous Linux. La compilation de `libwconr` et le linkage avec OpenSSL sont plus simples et mieux documentés sous un environnement POSIX.
- **Docker natif.** Docker fonctionne nativement sous WSL2 (via le kernel Linux intégré) sans nécessiter Docker Desktop, réduisant la consommation de ressources.
- **Cohérence avec la production.** Les miroirs tournent sous Linux. Développer sous WSL garantit que les scripts, chemins et comportements sont identiques entre l'environnement de développement et la production.

---

## Synthèse comparative

| Couche | Technologie choisie | Alternatives écartées | Raison principale du choix |
|---|---|---|---|
| Noyau | C | Rust, C++ | Contrôle mémoire, FFI natif, tradition système |
| Format binaire | .WIZARD (custom, ELF-inspired) | ZIP, JSON, NUPKG | Parsing O(1), intégrité intégrée, mmap-compatible |
| Protocole réseau | WCR (TCP custom) | HTTP/REST, gRPC | Frames binaires compactes, opcodes métier |
| Crypto | OpenSSL | libsodium, BoringSSL | Standard, API C native, disponibilité universelle |
| CLI | Python + ctypes | Go, Rust CLI | Prototypage rapide, zéro dépendance FFI externe |
| API | FastAPI | Flask, Django, Express | Async natif, typage Pydantic, légèreté |
| GUI Runtime | Electron | Tauri, Qt | Maturité écosystème, distribution Windows |
| GUI Framework | React + TypeScript | Vue, Svelte, Angular | Écosystème dominant, type-safety, composabilité |
| Composants UI | shadcn/ui | MUI, Ant Design, Chakra | Ownership du code, personnalisation thème custom |
| BDD | PostgreSQL + Prisma | SQLite, MySQL, MongoDB | ACID, type-safety, migrations déclaratives |
| Infra | Docker | VMs, bare-metal | Reproductibilité, isolation, scalabilité |
| Dev env | WSL + Git | Windows natif, MSYS2 | Toolchain POSIX, cohérence prod |

---

> **Document maintenu par** : Thomas — Dernière mise à jour : Juillet 2026

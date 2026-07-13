# WinConveyoR — Documentation Cryptographique

> Document technique détaillant les mécanismes cryptographiques implémentés dans WinConveyoR : stockage sécurisé des mots de passe API, sécurisation des communications WCR, et calcul des checksums d'intégrité.

---

## Table des matières

1. [Principes généraux](#principes-généraux)
2. [Stockage des mots de passe API](#stockage-des-mots-de-passe-api)
3. [Sécurisation du protocole WCR](#sécurisation-du-protocole-wcr)
4. [Calcul des checksums](#calcul-des-checksums)
5. [Dépendances cryptographiques](#dépendances-cryptographiques)
6. [Modèle de menaces](#modèle-de-menaces)

---

## Principes généraux

WinConveyoR manipule des données sensibles à plusieurs niveaux : authentification des utilisateurs auprès de l'API, transfert de paquets depuis les miroirs, et vérification de l'intégrité des fichiers `.WIZARD`. La stratégie cryptographique du projet repose sur trois principes :

- **Aucune implémentation maison.** Tous les algorithmes cryptographiques proviennent d'OpenSSL, une bibliothèque auditée et maintenue. Les erreurs d'implémentation cryptographique sont la première source de vulnérabilités dans les outils de distribution logicielle.
- **Défense en profondeur.** Chaque couche (stockage, transport, intégrité) applique ses propres mécanismes de protection indépendamment des autres.
- **Principe du moindre privilège.** Les secrets (mots de passe, clés) ne sont jamais exposés en clair dans les logs, les configurations, ou les messages d'erreur.

---

## Stockage des mots de passe API

### Contexte

L'API REST de WinConveyoR nécessite une authentification pour les opérations privilégiées (publication de paquets, administration des miroirs). Les mots de passe des comptes API doivent être stockés de manière sécurisée côté serveur.

### Algorithme de hachage — bcrypt

Les mots de passe ne sont jamais stockés en clair ni sous forme de hash simple. WinConveyoR utilise **bcrypt** pour le hachage des mots de passe :

```
hash_stocké = bcrypt(mot_de_passe, sel_aléatoire, coût)
```

#### Pourquoi bcrypt ?

- **Résistance au brute-force.** bcrypt est un algorithme volontairement lent (« key derivation function »). Le paramètre de coût (`work factor`) contrôle le nombre d'itérations internes, rendant chaque tentative de brute-force exponentiellement plus coûteuse.
- **Sel intégré.** Chaque hash bcrypt embarque un sel aléatoire de 128 bits, éliminant les attaques par rainbow tables. Deux utilisateurs avec le même mot de passe produisent des hash différents.
- **Résistance aux GPU.** L'algorithme Blowfish sous-jacent de bcrypt est memory-hard, ce qui rend l'accélération par GPU ou ASIC significativement moins efficace que pour SHA-256 ou MD5.

#### Paramètres de configuration

| Paramètre | Valeur | Justification |
|---|---|---|
| Work factor | 12 | ~250ms par hash sur matériel moderne, compromis sécurité/UX |
| Sel | 128 bits, `CSPRNG` | Généré via `RAND_bytes()` d'OpenSSL |
| Format de sortie | `$2b$12$...` | Format bcrypt standard, portable |

#### Pourquoi pas Argon2 ?

Argon2 (vainqueur du Password Hashing Competition 2015) offre une résistance supérieure aux attaques par GPU grâce à son paramètre de mémoire. Cependant, bcrypt est retenu pour WinConveyoR car :

- Son support natif dans OpenSSL simplifie l'intégration avec `libwconr`.
- Son format de hash est universellement reconnu par les outils d'audit et de migration.
- Le work factor 12 est suffisant pour le modèle de menaces d'un gestionnaire de paquets (le vecteur d'attaque principal n'est pas le brute-force de mots de passe API mais la compromission de paquets).

### Flux d'authentification

```
┌──────────┐        ┌──────────┐        ┌──────────────┐
│  Client  │        │   API    │        │  PostgreSQL  │
│ (CLI/GUI)│        │ (FastAPI)│        │              │
└────┬─────┘        └────┬─────┘        └──────┬───────┘
     │  POST /auth/login  │                     │
     │  { email, passwd } │                     │
     │───────────────────>│                     │
     │                    │  SELECT hash WHERE  │
     │                    │  email = ?          │
     │                    │────────────────────>│
     │                    │     hash_stocké     │
     │                    │<────────────────────│
     │                    │                     │
     │                    │  bcrypt_verify(      │
     │                    │    passwd,           │
     │                    │    hash_stocké)      │
     │                    │                     │
     │   JWT (si valide)  │                     │
     │<───────────────────│                     │
```

### Stockage côté client

Pour les sessions persistantes, le client stocke un **JWT (JSON Web Token)** signé, jamais le mot de passe :

- **Windows** : le token est stocké dans le **Windows Credential Manager** via l'API `CredWrite`/`CredRead`, qui chiffre les données avec DPAPI (Data Protection API), lié au compte Windows de l'utilisateur.
- **Linux/WSL** : le token est stocké dans un fichier `~/.config/wconr/credentials` avec permissions `600` (lecture/écriture propriétaire uniquement). Un avertissement est émis si les permissions sont trop permissives.
- **Variables d'environnement** : le token peut être passé via `WCONR_API_TOKEN` pour les environnements CI/CD, sans persistance sur disque.

Le JWT a une durée de vie configurable (défaut : 24h) et contient les claims suivants :

| Claim | Contenu | Rôle |
|---|---|---|
| `sub` | UUID de l'utilisateur | Identification |
| `exp` | Timestamp d'expiration | Invalidation automatique |
| `iat` | Timestamp d'émission | Détection de replay |
| `scope` | `publish`, `admin`, `read` | Contrôle d'accès |

Le JWT est signé avec **HMAC-SHA256** (`HS256`) côté serveur. La clé de signature est une valeur aléatoire de 256 bits générée au déploiement et stockée en variable d'environnement serveur (`WCONR_JWT_SECRET`).

---

## Sécurisation du protocole WCR

### Contexte

Le protocole WCR (WinConveyoR Remote) transporte des paquets `.WIZARD` et des métadonnées de synchronisation entre les clients et les serveurs miroir. Ce canal doit garantir la confidentialité, l'intégrité et l'authenticité des échanges.

### Schéma de chiffrement hybride — RSA + AES-256

WCR utilise un schéma de **chiffrement hybride** combinant RSA (asymétrique) pour l'échange de clés et AES-256 (symétrique) pour le chiffrement des données :

```
┌──────────────────────────────────────────────────┐
│              Chiffrement hybride WCR             │
│                                                  │
│   RSA (asymétrique)     AES-256 (symétrique)     │
│   ─────────────────     ────────────────────     │
│   Échange de clés   →   Chiffrement des données │
│   Lent, sécurisé        Rapide, performant       │
└──────────────────────────────────────────────────┘
```

#### Pourquoi un schéma hybride ?

Le chiffrement asymétrique (RSA) et symétrique (AES) ont des forces complémentaires. Les combiner permet d'exploiter les avantages de chacun tout en compensant leurs faiblesses respectives :

- **RSA seul est trop lent.** Le chiffrement RSA est environ 1000x plus lent qu'AES pour le traitement de données brutes. Chiffrer des paquets `.WIZARD` de plusieurs Mo en RSA pur serait prohibitif en termes de performance.
- **AES seul pose un problème de distribution de clé.** AES est un algorithme symétrique : les deux parties doivent posséder la même clé secrète. Sans mécanisme d'échange sécurisé, cette clé devrait être partagée à l'avance, ce qui est impraticable pour un réseau ouvert de miroirs.
- **RSA + AES résout les deux problèmes.** RSA sécurise l'échange d'une clé AES éphémère, puis AES chiffre les données à haute vitesse avec cette clé.

### RSA — Échange de clés

#### Fonctionnement

Chaque serveur miroir possède une **paire de clés RSA** (clé publique + clé privée). La clé publique est distribuée aux clients, la clé privée reste exclusivement sur le serveur.

Lors de l'établissement d'une session WCR :

1. Le client génère une **clé AES-256 aléatoire** (256 bits) pour la session, appelée **session key**.
2. Le client chiffre cette session key avec la **clé publique RSA** du serveur miroir.
3. Le client envoie la session key chiffrée au serveur.
4. Le serveur déchiffre la session key avec sa **clé privée RSA**.
5. Les deux parties possèdent maintenant la même clé AES-256 pour la suite de la communication.

```
┌──────────┐                              ┌──────────┐
│  Client  │                              │  Miroir  │
└────┬─────┘                              └────┬─────┘
     │                                         │
     │         1. Demande clé publique RSA     │
     │────────────────────────────────────────>│
     │                                         │
     │         2. Clé publique RSA             │
     │<────────────────────────────────────────│
     │                                         │
     │  3. Génère session key AES-256          │
     │     (CSPRNG — RAND_bytes)               │
     │                                         │
     │  4. Chiffre session key avec RSA pub    │
     │                                         │
     │         5. Session key chiffrée (RSA)   │
     │────────────────────────────────────────>│
     │                                         │
     │                    6. Déchiffre avec RSA priv
     │                                         │
     │  ═══════════════════════════════════════ │
     │     Session AES-256 établie             │
     │     Toutes les frames suivantes         │
     │     sont chiffrées en AES-256           │
     │  ═══════════════════════════════════════ │
```

#### Paramètres RSA

| Paramètre | Valeur | Justification |
|---|---|---|
| Taille de clé | 2048 bits (minimum), 4096 bits (recommandé) | 2048 bits offre ~112 bits de sécurité, suffisant jusqu'à ~2030. 4096 bits offre une marge pour les miroirs long-terme. |
| Padding | OAEP (Optimal Asymmetric Encryption Padding) | PKCS#1 v1.5 est vulnérable aux attaques de Bleichenbacher. OAEP (RSA-OAEP avec SHA-256) est le standard moderne recommandé. |
| Implémentation | OpenSSL `EVP_PKEY_encrypt` / `EVP_PKEY_decrypt` | API EVP pour l'agilité algorithmique et l'accélération matérielle. |

#### Pourquoi RSA et pas ECDH (courbes elliptiques) ?

RSA est un algorithme éprouvé depuis 1977, massivement audité, et dont les propriétés de sécurité sont bien comprises. Les courbes elliptiques (ECDH/ECDSA) offrent des clés plus courtes pour un niveau de sécurité équivalent, mais RSA est retenu pour WinConveyoR car :

- Sa simplicité conceptuelle facilite l'audit et le debugging du protocole.
- Son support dans OpenSSL est mature et stable sur toutes les plateformes cibles.
- La taille des clés n'est pas un facteur limitant pour WCR (l'échange de clés est ponctuel, pas répété par frame).

### AES-256 — Chiffrement des données

#### Fonctionnement

Une fois la session key échangée via RSA, toutes les frames WCR sont chiffrées avec **AES-256** en mode **CBC** (Cipher Block Chaining) ou **GCM** (Galois/Counter Mode) :

```
┌────────────┬──────────┬────────┬──────────────────────────────┐
│  Opcode    │  Length  │   IV   │   Payload chiffré (AES-256)  │
│  (1 byte)  │ (4 bytes)│(16 B)  │         (N bytes)            │
└────────────┴──────────┴────────┴──────────────────────────────┘
```

Chaque frame contient un **IV (Initialization Vector)** unique de 16 octets (128 bits), généré aléatoirement via `RAND_bytes()`. L'IV garantit que deux frames identiques produisent des ciphertexts différents, empêchant les attaques par analyse de patterns.

#### Pourquoi AES-256 ?

- **Standard mondial.** AES (Advanced Encryption Standard) est le standard de chiffrement symétrique adopté par le NIST en 2001. Il est utilisé par les gouvernements, les institutions financières, et la quasi-totalité des protocoles de communication sécurisés.
- **Sécurité 256 bits.** AES-256 offre une marge de sécurité maximale parmi les variantes AES (128, 192, 256). Même avec les avancées théoriques (attaques biclique), la complexité effective reste à ~254.4 bits, largement hors de portée.
- **Performance.** Les processeurs modernes disposent d'instructions dédiées AES-NI (Intel/AMD) qui accélèrent le chiffrement/déchiffrement AES de manière matérielle, atteignant des débits de plusieurs Go/s. C'est essentiel pour le transfert de paquets `.WIZARD` volumineux.
- **Blocs de 128 bits.** AES opère sur des blocs de 128 bits (16 octets), ce qui est adapté aux frames WCR binaires sans overhead excessif de padding.

#### Modes de chiffrement

| Mode | Avantages | Inconvénients | Usage dans WCR |
|---|---|---|---|
| **CBC** (Cipher Block Chaining) | Simple, éprouvé, largement supporté | Nécessite un padding (PKCS#7), pas d'authentification intégrée | Chiffrement des frames de données standard |
| **GCM** (Galois/Counter Mode) | Authentification intégrée (AEAD), parallélisable | Légèrement plus complexe à implémenter | Chiffrement des frames critiques (auth, metadata) |

En mode GCM, un **tag d'authentification** de 128 bits est ajouté à chaque frame, fournissant à la fois le chiffrement et la vérification d'intégrité en une seule opération. C'est le mode privilégié pour les échanges sensibles (authentification, transfert de checksums).

#### Pourquoi pas ChaCha20-Poly1305 ?

ChaCha20-Poly1305 est une alternative performante sur les machines sans accélération AES-NI. Cependant, AES-256 est retenu car WinConveyoR cible principalement Windows sur architecture x86/x64, où AES-NI est quasi universellement disponible. ChaCha20 n'apporte pas de bénéfice significatif sur cette plateforme.

### Cycle de vie d'une session WCR

```
  Connexion TCP
       │
       ▼
  ┌─────────────────────────────┐
  │  Phase 1 — Échange de clés  │
  │  (RSA)                      │
  │                             │
  │  • Client récupère clé pub  │
  │  • Client génère session key│
  │  • Chiffrement RSA-OAEP     │
  │  • Envoi au serveur         │
  │  • Serveur déchiffre        │
  └──────────────┬──────────────┘
                 │
                 ▼
  ┌─────────────────────────────┐
  │  Phase 2 — Communication    │
  │  (AES-256)                  │
  │                             │
  │  • Toutes les frames sont   │
  │    chiffrées avec la        │
  │    session key              │
  │  • IV unique par frame      │
  │  • Opcodes : SYNC, FETCH,   │
  │    VERIFY, PUBLISH          │
  └──────────────┬──────────────┘
                 │
                 ▼
  ┌─────────────────────────────┐
  │  Phase 3 — Fermeture        │
  │                             │
  │  • Session key détruite     │
  │    (zéroisation mémoire)    │
  │  • Connexion TCP fermée     │
  └─────────────────────────────┘
```

La session key est **éphémère** : elle est générée pour chaque connexion et détruite à la fermeture. En cas de compromission d'une session key, seule la session correspondante est exposée — les sessions passées et futures restent protégées.

### Gestion des clés RSA des miroirs

| Aspect | Implémentation |
|---|---|
| Génération | `openssl genrsa -out mirror_private.pem 4096` au déploiement du miroir |
| Stockage clé privée | Fichier `.pem` avec permissions `400`, accessible uniquement par le service WCR |
| Distribution clé publique | Embarquée dans la configuration client ou récupérée au premier contact |
| Rotation | Recommandée tous les 12 mois, avec période de grâce où l'ancienne et la nouvelle clé sont acceptées |

### Résumé des protections par couche

| Couche | Mécanisme | Protège contre |
|---|---|---|
| Échange de clés | RSA-OAEP (2048/4096 bits) | Interception de la session key, MITM |
| Chiffrement données | AES-256-CBC / AES-256-GCM | Écoute passive, analyse de trafic |
| Intégrité (mode GCM) | Tag d'authentification AEAD 128 bits | Altération des frames en transit |
| Session key éphémère | Clé AES générée par session, détruite à la fermeture | Compromission rétrospective |
| IV unique par frame | 128 bits aléatoires (`RAND_bytes`) | Attaques par analyse de patterns |

---

## Calcul des checksums

### Contexte

Chaque paquet `.WIZARD` doit être vérifié pour garantir que son contenu n'a pas été altéré entre la publication et l'installation. Le checksum est le mécanisme central de cette vérification.

### Algorithme — SHA-256

WinConveyoR utilise **SHA-256** (SHA-2 family, 256 bits) pour tous les calculs de checksums :

#### Pourquoi SHA-256 ?

- **Résistance aux collisions.** Aucune collision SHA-256 n'a été trouvée à ce jour. La complexité théorique d'une attaque par anniversaire est de 2^128 opérations, bien au-delà de toute capacité de calcul actuelle.
- **Standard universel.** SHA-256 est utilisé par Git, Bitcoin, TLS, et la majorité des gestionnaires de paquets (apt, npm, pip). Son adoption massive signifie qu'il est constamment scruté et audité par la communauté cryptographique.
- **Performance.** SHA-256 bénéficie d'extensions matérielles (Intel SHA-NI, ARM SHA2) sur les processeurs modernes, permettant un débit de hachage de plusieurs Go/s.
- **Taille de digest adaptée.** 256 bits (32 octets) offrent un niveau de sécurité suffisant (128 bits de résistance aux collisions) tout en restant compact dans les structures binaires et les affichages hexadécimaux (64 caractères).

#### Pourquoi pas SHA-512 ou SHA-3 ?

SHA-512 offre une marge de sécurité supérieure mais au prix d'un digest plus volumineux (64 octets vs 32) qui augmente la taille des index de paquets sans bénéfice pratique pour le modèle de menaces de WinConveyoR. SHA-3 (Keccak) est une alternative viable mais son support matériel est moins répandu que SHA-2, et l'écosystème (outils, documentation) est plus mature autour de SHA-256.

### Implémentation dans libwconr

Le calcul de checksum est implémenté dans `libwconr` via l'API EVP d'OpenSSL :

```c
#include <openssl/evp.h>

int wconr_checksum_compute(const uint8_t *data, size_t len,
                           uint8_t out[32])
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
        return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, data, len)            != 1 ||
        EVP_DigestFinal_ex(ctx, out, NULL)           != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);
    return 0;
}
```

L'API EVP (Envelope) est préférée aux appels directs `SHA256()` pour deux raisons :

- **Agilité algorithmique.** Si SHA-256 devait être remplacé à l'avenir, seul le paramètre `EVP_sha256()` change. Le reste du code reste identique.
- **Accélération matérielle transparente.** L'API EVP détecte et utilise automatiquement les extensions matérielles SHA-NI si disponibles, sans code spécifique.

### Points de vérification

Le checksum SHA-256 intervient à trois moments distincts dans le cycle de vie d'un paquet :

```
   Publication                    Transfert                   Installation
┌──────────────┐             ┌──────────────┐             ┌──────────────┐
│  Auteur du   │             │   Miroir /   │             │   Client     │
│   paquet     │             │   Serveur    │             │  (CLI/GUI)   │
└──────┬───────┘             └──────┬───────┘             └──────┬───────┘
       │                            │                            │
       │  1. Calcul du hash         │                            │
       │     sur le .WIZARD         │                            │
       │     complet                │                            │
       │                            │                            │
       │  Hash inscrit dans         │                            │
       │  le manifest + signé       │                            │
       │──────────────────────────>│                            │
       │                            │                            │
       │                            │  2. Vérification à la     │
       │                            │     réception (hash du    │
       │                            │     fichier reçu vs hash  │
       │                            │     du manifest)          │
       │                            │                            │
       │                            │──────────────────────────>│
       │                            │                            │
       │                            │                            │  3. Vérification
       │                            │                            │     avant install
       │                            │                            │     (hash du fichier
       │                            │                            │     local vs hash
       │                            │                            │     de l'index)
```

#### Étape 1 — Publication

Lors du `wconr publish`, le hash SHA-256 du fichier `.WIZARD` complet est calculé et inscrit dans le **manifest du paquet** (section dédiée du format binaire). Ce hash est également enregistré dans l'index du miroir.

#### Étape 2 — Synchronisation miroir

Lorsqu'un miroir secondaire synchronise un paquet depuis le miroir primaire via WCR, il recalcule le hash du fichier reçu et le compare au hash annoncé dans l'index. En cas de mismatch, le paquet est rejeté et un retry est déclenché.

#### Étape 3 — Installation client

Avant l'extraction et l'installation, le client recalcule le hash du fichier `.WIZARD` téléchargé et le compare au hash stocké dans son index local (obtenu lors du dernier `wconr sync`). L'installation est bloquée si les hash ne correspondent pas, avec un message d'erreur explicite :

```
error: checksum mismatch for package 'example-pkg-1.2.0.wizard'
  expected: a3f2b8c1...d4e5f6a7
  got:      9b8c7d6e...f0a1b2c3
  The package may have been corrupted or tampered with.
  Run 'wconr sync' to refresh the package index.
```

### Checksums dans le format .WIZARD

Le hash SHA-256 est stocké dans la **section checksum** du fichier `.WIZARD`, à un offset fixe défini dans le header :

```
┌──────────────────────────────────────────────┐
│              .WIZARD Header                  │
│  ┌─────────────────────────────────────────┐ │
│  │ magic: 0x57495A41 ("WIZA")             │ │
│  │ version: uint16                         │ │
│  │ section_table_offset: uint32            │ │
│  │ section_count: uint16                   │ │
│  └─────────────────────────────────────────┘ │
├──────────────────────────────────────────────┤
│           Section: METADATA                  │
│  (nom, version, auteur, description, ...)    │
├──────────────────────────────────────────────┤
│           Section: CHECKSUM                  │
│  ┌─────────────────────────────────────────┐ │
│  │ algorithm: uint8 (0x01 = SHA-256)       │ │
│  │ digest: uint8[32]                       │ │
│  │ scope: uint8 (0x01 = payload only)      │ │
│  └─────────────────────────────────────────┘ │
├──────────────────────────────────────────────┤
│           Section: PAYLOAD                   │
│  (fichiers du paquet, compressés)             │
└──────────────────────────────────────────────┘
```

Le champ `algorithm` permet une migration future vers un autre algorithme sans casser le format : le parser sélectionne la fonction de hash en fonction de cette valeur. Le champ `scope` indique si le hash couvre uniquement le payload ou l'intégralité du fichier (header + metadata + payload), permettant des stratégies de vérification différentes selon le contexte.

---

## Dépendances cryptographiques

| Fonction | Bibliothèque | Version minimale | Justification |
|---|---|---|---|
| SHA-256 (checksums) | OpenSSL (libcrypto) | 1.1.1+ | API EVP stable, accélération matérielle |
| bcrypt (passwords) | OpenSSL (libcrypto) | 1.1.1+ | Intégration native C, pas de dépendance supplémentaire |
| RSA (échange de clés WCR) | OpenSSL (libcrypto) | 1.1.1+ | `EVP_PKEY_encrypt` / `EVP_PKEY_decrypt`, support OAEP |
| AES-256 (chiffrement WCR) | OpenSSL (libcrypto) | 1.1.1+ | `EVP_EncryptInit_ex` / `EVP_DecryptInit_ex`, accélération AES-NI |
| JWT (tokens) | PyJWT (Python) | 2.0+ | Signing/verification côté API FastAPI |
| CSPRNG (sel, clés, IV) | OpenSSL `RAND_bytes` | 1.1.1+ | Source d'entropie cryptographique (session keys, IV AES, sel bcrypt) |

La version minimale d'OpenSSL 1.1.1 est imposée car c'est la première version avec une API EVP complète et stable pour RSA-OAEP et AES-GCM. Les versions antérieures sont refusées au `configure` / build time.

---

## Modèle de menaces

| Menace | Vecteur d'attaque | Protection |
|---|---|---|
| Interception réseau | Écoute passive du trafic client-miroir | AES-256 (chiffrement de toutes les frames WCR) |
| Man-in-the-middle | Usurpation d'un miroir, interception de la session key | RSA-OAEP (seul le miroir possède la clé privée pour déchiffrer la session key) |
| Compromission rétrospective | Vol d'une session key passée | Session key éphémère (détruite à chaque fermeture de connexion) |
| Paquet corrompu | Altération en transit ou sur disque | SHA-256 checksum à 3 points de vérification |
| Paquet malveillant | Publication d'un paquet compromis par un attaquant | Hash signé dans le manifest + authentification API |
| Brute-force mot de passe | Tentatives massives sur l'API `/auth/login` | bcrypt (work factor 12) + rate limiting API |
| Vol de token | Accès au fichier credentials ou à la mémoire | DPAPI (Windows), permissions 600 (Linux), expiration JWT |
| Analyse de patterns | Frames identiques produisant le même ciphertext | IV unique de 128 bits par frame (CSPRNG) |
| Altération de frame (mode GCM) | Modification d'une frame chiffrée en transit | Tag d'authentification AEAD 128 bits |
| Rainbow table | Hash de mots de passe sans sel | Sel aléatoire 128 bits intégré à bcrypt |
| Compromission clé RSA miroir | Vol de la clé privée d'un miroir | Rotation de clés recommandée tous les 12 mois, clé privée permissions 400 |

---

> **Document maintenu par** : Thomas — Dernière mise à jour : Juillet 2026

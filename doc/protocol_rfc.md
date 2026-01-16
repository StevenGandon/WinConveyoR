# RFC - WinConveyoR Communication Protocol (WCP)

## 1. Introduction

Le protocole WCR (WinConveyoR) est un protocole de communication binaire basé sur TCP pour la gestion de dépôts de packages logiciels.

### 1.1 Conventions

Les mots-clés "DOIT", "NE DOIT PAS", "REQUIS", "DEVRA", "PEUT" sont interprétés comme décrit dans RFC 2119.

## 2. Architecture du Protocole

### 2.1 Couche Transport

- **Port par défaut**: 1674
- **Transport**: TCP
- **Encodage**: UTF-8

### 2.2 Format des Messages

Tous les messages suivent ce format binaire:

```
+--------+--------+----------------+------------------+
| MAGIC  | FLAGS  | PAYLOAD_SIZE   | PAYLOAD_CONTENT  |
| 4 bytes| 2 bytes| 8 bytes        | variable         |
+--------+--------+----------------+------------------+
```

#### 2.2.1 Champs du Message

- **MAGIC** (4 octets): Valeur fixe `0xffc407ec` (big-endian)
- **FLAGS** (2 octets): Réservé pour usage futur (actuellement `0x0000`)
- **PAYLOAD_SIZE** (8 octets): Taille du contenu en octets (big-endian)
- **PAYLOAD_CONTENT** (variable): Contenu JSON encodé en UTF-8

#### 2.2.2 Limites

- Taille maximale du payload: 2 Go (2,147,483,648 octets)
- Les messages dépassant cette limite DOIVENT être rejetés avec une erreur `BufferError`

### 2.3 Format JSON du Payload

Tous les payloads DOIVENT être des objets JSON avec cette structure:

```json
{
  "action": "string",
  "code": 0,
  "data": {}
}
```

#### 2.3.1 Champs JSON

- **action** (string, REQUIS): Type d'action ou de réponse
- **code** (integer, REQUIS): Code de statut (0 = succès, 1 = erreur)
- **data** (object, REQUIS): Données spécifiques à l'action

## 3. Cycle de Vie de la Connexion

### 3.1 Établissement de Connexion

1. Le client établit une connexion TCP au serveur
2. Le client envoie un message `hello`
3. Le serveur répond avec un message `hello` contenant le message de bienvenue
4. La connexion est établie

### 3.2 Session

1. Le client envoie un message `connect` avec authentification
2. Le serveur crée une session et retourne un `session_id`
3. Le client utilise ce `session_id` pour toutes les opérations protégées

### 3.3 Terminaison

1. Le client envoie un message `goodbye`
2. Le serveur répond et ferme la connexion
3. Toutes les sessions associées sont nettoyées

## 4. Actions du Protocole

### 4.1 hello - Handshake Initial

**Direction**: Client → Serveur → Client

**Requête Client**:
```json
{
  "action": "hello",
  "data": {}
}
```

**Réponse Serveur**:
```json
{
  "action": "hello",
  "code": 0,
  "data": {
    "msg": "******************************\n*                            *\n*      wcr source server     *\n*                            *\n******************************"
  }
}
```

### 4.2 connect - Authentification

**Direction**: Client → Serveur → Client

**Requête Client**:
```json
{
  "action": "connect",
  "data": {
    "password": "string"
  }
}
```

**Réponse Serveur (Succès)**:
```json
{
  "action": "connect",
  "code": 0,
  "data": {
    "msg": "ok",
    "session_id": 123456789
  }
}
```

**Réponse Serveur (Échec)**:
```json
{
  "action": "connect",
  "code": 1,
  "data": {
    "msg": "invalid_password"
  }
}
```

### 4.3 list_packages - Lister les Packages

**Route Protégée**: OUI

**Requête Client**:
```json
{
  "action": "list_packages",
  "data": {
    "session_id": 123456789
  }
}
```

**Réponse Serveur**:
```json
{
  "action": "list_packages",
  "code": 0,
  "data": {
    "packages": ["gcc", "python", "nodejs"]
  }
}
```

### 4.4 write - Sauvegarder les Modifications

**Route Protégée**: OUI

**Requête Client**:
```json
{
  "action": "write",
  "data": {
    "session_id": 123456789
  }
}
```

**Réponse Serveur**:
```json
{
  "action": "write",
  "code": 0,
  "data": {
    "msg": "ok",
    "edited_package_count": 5,
    "edited_instance_count": 12
  }
}
```

### 4.5 disconnect - Fermer une Session

**Route Protégée**: OUI

**Requête Client**:
```json
{
  "action": "disconnect",
  "data": {
    "session_id": 123456789
  }
}
```

**Réponse Serveur**:
```json
{
  "action": "disconnect",
  "code": 0,
  "data": {
    "msg": "ok"
  }
}
```

### 4.6 goodbye - Terminer la Connexion

**Direction**: Client → Serveur → Client

**Requête Client**:
```json
{
  "action": "goodbye",
  "data": {}
}
```

**Réponse Serveur**:
```json
{
  "action": "goodbye",
  "code": 0,
  "data": {
    "msg": "goodbye"
  }
}
```

## 5. Codes d'Erreur

### 5.1 Codes de Statut

- **0**: Succès
- **1**: Erreur

### 5.2 Messages d'Erreur Standard

- `"ko"`: Requête malformée (données manquantes)
- `"invalid_password"`: Mot de passe incorrect
- `"invalid_session_id"`: Session inexistante
- `"unauthorized_session"`: Session n'appartenant pas au client
- `"action not found: '{action}'"`: Action non reconnue
- `"failed to parse json message"`: JSON invalide
- `"missing fields in json"`: Champs requis manquants

## 6. Sécurité

### 6.1 Authentification

- Mot de passe envoyé en clair via variable d'environnement `WCR_PASSWORD`
- **Note**: Cette implémentation n'est PAS sécurisée pour la production

### 6.2 Sessions

- Chaque session est liée à un client spécifique
- Les sessions sont identifiées par un UUID (128 bits)
- Les sessions expirent automatiquement à la déconnexion du client

### 6.3 Routes Protégées

Les actions suivantes nécessitent une authentification:
- `list_packages`
- `write`
- `disconnect`

## 7. Recommandations d'Implémentation

### 7.1 Clients

- DOIVENT valider le magic number avant de traiter un message
- DOIVENT respecter la limite de taille de payload
- DEVRAIENT implémenter un timeout pour les opérations réseau
- DEVRAIENT gérer la reconnexion automatique

### 7.2 Serveurs

- DOIVENT rejeter les messages avec magic number invalide
- DOIVENT limiter le nombre de connexions simultanées
- DEVRAIENT logger toutes les opérations d'authentification
- DEVRAIENT implémenter un rate limiting

## 8. Extensions Futures

### 8.1 Propositions

- Chiffrement TLS/SSL
- Authentification par clé RSA
- Compression du payload
- Streaming de packages volumineux
- Checksum de message

### 8.2 Flags Réservés

Les 2 octets de flags sont réservés pour ces extensions futures.

## 9. Exemple de Session Complète

```
Client → Server: hello
Server → Client: hello (avec message de bienvenue)
Client → Server: connect (avec password)
Server → Client: connect (avec session_id)
Client → Server: list_packages (avec session_id)
Server → Client: list_packages (avec liste)
Client → Server: write (avec session_id)
Server → Client: write (avec compteurs)
Client → Server: disconnect (avec session_id)
Server → Client: disconnect (confirmation)
Client → Server: goodbye
Server → Client: goodbye
[Connexion fermée]
```

## 10. Références

- RFC 2119: Key words for use in RFCs
- JSON: RFC 8259
- TCP: RFC 793
- UTF-8: RFC 3629

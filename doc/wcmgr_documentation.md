# Documentation wcmgr - WinConveyoR Manager CLI

## Vue d'ensemble

`wcmgr` est l'outil en ligne de commande pour gérer un serveur miroir WinConveyoR. Il permet de se connecter au serveur, gérer les packages et effectuer des opérations administratives.

## Installation

Le client est disponible dans le container `custom-server` :

```bash
# Depuis l'intérieur du container
wcmgr
```

## Architecture

```
┌─────────────────────────────────────────────┐
│          NetworkCLI (Interface)             │
│  - Gestion du terminal                      │
│  - Historique des commandes                 │
│  - Prompt interactif                        │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│         ClientSocket (Network)              │
│  - Connexion TCP au serveur                 │
│  - Protocole WCR                            │
│  - Gestion des sessions                     │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│        wcrmirror Server (Port 1674)         │
│  - Authentification                         │
│  - Gestion des packages                     │
│  - Opérations CRUD                          │
└─────────────────────────────────────────────┘
```

## Commandes

### connect

Authentifie l'utilisateur et crée une nouvelle session.

**Syntaxe** :
```
connect <password>
```

**Arguments** :
- `password` (obligatoire) : Mot de passe du serveur

**Exemple** :
```
>>> connect mypassword
```

**Réponse** :
- Succès : Le prompt change pour afficher l'ID de session `[<session_id>]>>>`
- Échec : Affiche un message d'erreur

**Comportement** :
1. Envoie la requête d'authentification au serveur
2. Reçoit un `session_id` en cas de succès
3. Crée une session locale
4. Change le prompt pour inclure l'ID de session
5. Rend disponibles les commandes protégées

---

### disconnect

Ferme une session active.

**Syntaxe** :
```
disconnect <session_id>
```

**Arguments** :
- `session_id` (obligatoire) : ID numérique de la session à fermer

**Exemple** :
```
[a1b2c3]>>> disconnect 12345
```

**Comportement** :
1. Envoie la requête de déconnexion au serveur
2. Supprime la session côté serveur
3. Supprime la session locale
4. Si c'était la session active, réinitialise le prompt à `>>>`

**Note** : Commande protégée - nécessite d'être dans une session

---

### list_sessions

Affiche toutes les sessions locales actives.

**Syntaxe** :
```
list_sessions
```

**Arguments** : Aucun

**Exemple** :
```
>>> list_sessions
a1b2c3
d4e5f6
g7h8i9
```

**Sortie** :
Liste des IDs de session en hexadécimal, un par ligne.

**Note** : N'affiche que les sessions locales, pas celles du serveur

---

### switch_session

Change la session active ou revient au mode sans session.

**Syntaxe** :
```
switch_session [session_id]
```

**Arguments** :
- `session_id` (optionnel) : ID hexadécimal de la session

**Exemples** :
```
>>> switch_session a1b2c3
[a1b2c3]>>>

[a1b2c3]>>> switch_session
>>>
```

**Comportement** :
- Avec argument : Active la session spécifiée et change le prompt
- Sans argument : Désactive la session active et revient au prompt de base
- La session doit exister localement

---

### list_packages

Liste tous les packages disponibles dans le dépôt.

**Syntaxe** :
```
list_packages <session_id>
```

**Arguments** :
- `session_id` (obligatoire) : ID numérique de la session

**Exemple** :
```
[a1b2c3]>>> list_packages 12345
['gcc', 'python', 'nodejs', 'docker']
```

**Sortie** :
Liste Python des noms de packages disponibles.

**Note** : Commande protégée - nécessite une session valide

---

### write

Sauvegarde toutes les modifications dans le système de fichiers.

**Syntaxe** :
```
write <session_id>
```

**Arguments** :
- `session_id` (obligatoire) : ID numérique de la session

**Exemple** :
```
[a1b2c3]>>> write 12345
3 PACKAGE INSTANCE IN 2 PACKAGE WRITTEN
```

**Sortie** :
Affiche le nombre d'instances de packages et de packages écrits.

**Comportement** :
1. Demande au serveur de persister les modifications
2. Le serveur crée un backup timestampé
3. Écrit tous les fichiers modifiés (pkgs.list, register/, pkgs/)
4. Recalcule le checksum global
5. Retourne les statistiques

**Note** : Commande protégée - nécessite une session valide

---

### quit

Quitte l'application proprement.

**Syntaxe** :
```
quit
```

**Arguments** : Aucun

**Exemple** :
```
>>> quit
```

**Comportement** :
1. Envoie un message `goodbye` au serveur
2. Ferme toutes les sessions locales
3. Ferme la connexion TCP
4. Restaure les paramètres du terminal
5. Quitte l'application

---

## Fonctionnalités Avancées

### Historique des Commandes

L'interface conserve un historique des commandes saisies :

- **Flèche Haut / Flèche Bas** : Naviguer dans l'historique
- **Ctrl+C / Ctrl+Z** : Quitter l'application
- Historique persistant durant toute la session

### Navigation dans la Ligne

- **Flèche Gauche / Flèche Droite** : Déplacer le curseur
- **Backspace / Delete** : Supprimer des caractères
- **Home** : Début de ligne
- **End** : Fin de ligne

### Prompts

- **Base** : `>>> ` - Mode sans session
- **Session** : `[<session_id>]>>> ` - Mode avec session active

### Middlewares

Toutes les commandes réseau passent par des middlewares de validation :

1. **check_response** : Vérifie que la réponse est du JSON valide
2. **check_status** : Vérifie le code de statut (0 = succès, 1 = erreur)

En cas d'échec, un message d'erreur descriptif est affiché.

## Gestion des Erreurs

### Erreurs de Connexion

```
failed to connect to server. (Connection refused)
```
Le serveur n'est pas accessible ou n'écoute pas sur le port.

### Erreurs d'Authentification

```
command failed: invalid_password
```
Le mot de passe fourni est incorrect.

### Erreurs de Session

```
command need to be executed in a session.
```
La commande nécessite d'être dans une session active.

```
command failed: invalid_session_id
```
L'ID de session n'existe pas sur le serveur.

```
command failed: unauthorized_session
```
La session appartient à un autre client.

### Erreurs de Commande

```
command not found 'xyz'.
```
La commande saisie n'existe pas.

```
not enough arguments, mandatory arguments are: ('password',)
```
Des arguments obligatoires sont manquants.

```
too much arguments argument list is: ('password',)
```
Trop d'arguments ont été fournis.

## Variables d'Environnement

Le client se connecte par défaut à :
- **Host** : 127.0.0.1
- **Port** : 1674

Ces valeurs sont codées en dur dans `ClientSocket`.

## Exemple de Session Complète

```bash
# Démarrage du client
$ wcmgr

******************************
*                            *
*      wcr source server     *
*                            *
******************************

>>> connect mypassword
[a1b2c3]>>> list_packages 171234567
['gcc', 'python', 'nodejs']

[a1b2c3]>>> write 171234567
3 PACKAGE INSTANCE IN 3 PACKAGE WRITTEN

[a1b2c3]>>> list_sessions
a1b2c3

[a1b2c3]>>> switch_session
>>> disconnect 171234567

>>> quit
```

## Notes Techniques

### Format des Session IDs

- Générés via `uuid4().int` (UUID version 4 converti en entier)
- Affichés en hexadécimal sans le préfixe `0x`
- Exemple : `a1b2c3d4e5f6` au lieu de `0xa1b2c3d4e5f6`

### Terminal

- Mode raw activé pour capturer toutes les touches
- Compatible Linux/macOS (termios) et Windows (msvcrt)
- Restauration automatique des paramètres à la sortie

### Réseau

- Protocole personnalisé (WCR) sur TCP
- Messages binaires avec magic number `0xffc407ec`
- Payload JSON encodé en UTF-8
- Timeout implicite sur les opérations réseau

## Dépannage

### Le prompt ne s'affiche pas correctement

Vérifiez que le terminal supporte les séquences ANSI.

### Les flèches ne fonctionnent pas sous Windows

Utilisez les alternatives Windows : Flèche Haut = Ctrl+P, Flèche Bas = Ctrl+N

### "Lost connection to the server"

Le serveur a fermé la connexion. Redémarrez wcmgr et reconnectez-vous.

### Les commandes protégées ne fonctionnent pas

Assurez-vous d'être dans une session active (prompt avec `[session_id]`).

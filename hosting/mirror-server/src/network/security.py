from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import serialization, hashes

class SecurityKey(object):
    def __init__(self, key):
        self.key = key

class PrivateSecurityKey(SecurityKey):
    @staticmethod
    def from_file(path, password: str = None, /, encoding = "utf8"):
        with open(path, "rb") as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=password.encode(encoding) if password is not None else b"None",
            )

        return (PrivateSecurityKey(private_key))

    @staticmethod
    def generate():
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
        )

        return (PrivateSecurityKey(private_key))

    def write(self, path, password: str = None, /, encoding = "utf8"):
        pem = self.key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.BestAvailableEncryption(password.encode(encoding) if password is not None else b"None")
        )

        with open(path, 'wb') as fp:
            fp.write(pem)

    def decrypt(self, message: bytes):
        return self.key.decrypt(
            message,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )

class PublicSecurityKey(SecurityKey):
    @staticmethod
    def from_file(path):
        with open(path, "rb") as key_file:
            public_key = serialization.load_pem_public_key(
                key_file.read()
            )

        return (PublicSecurityKey(public_key))

    @staticmethod
    def generate(private_key: PrivateSecurityKey):
        public_key = private_key.key.public_key()

        return (PublicSecurityKey(public_key))
    
    def write(self, path):
        pem = self.key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        with open(path, 'wb') as fp:
            fp.write(pem)

    def encrypt(self, message: bytes):
        return self.key.encrypt(
            message,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
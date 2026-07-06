from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding as sym_padding
from os import urandom

class SecurityKey(object):
    def __init__(self, key):
        self.key = key

class PrivateSecurityKey(SecurityKey):
    def __init__(self, key):
        super().__init__(key)

    @staticmethod
    def from_file(path, password: str = None, /, encoding = "utf8"):
        with open(path, "rb") as key_file:
            private_key = serialization.load_pem_private_key(
                key_file.read(),
                password=password.encode(encoding) if password is not None else b"None",
            )

        return (PrivateSecurityKey(private_key))

    @staticmethod
    def from_string(string: str, password: str = None, /, encoding = "utf8"):
        private_key = serialization.load_pem_private_key(
                string.encode(encoding),
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

    def to_string(self, password: str = None, /, encoding = "utf8"):
        return (self.key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.BestAvailableEncryption(password.encode(encoding) if password is not None else b"None")
        ).decode(encoding))

    # def decrypt(self, message: bytes):
    #     return self.key.decrypt(
    #         message,
    #         padding.OAEP(
    #             mgf=padding.MGF1(algorithm=hashes.SHA256()),
    #             algorithm=hashes.SHA256(),
    #             label=None
    #         )
    #     )

    def decrypt(self, message: bytes):
        key_size_bytes = self.key.key_size // 8
        encrypted_key = message[:key_size_bytes]
        iv = message[key_size_bytes:key_size_bytes+16]
        ciphertext = message[key_size_bytes+16:]

        aes_key = self.key.decrypt(
            encrypted_key,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )

        cipher = Cipher(algorithms.AES(aes_key), modes.CBC(iv))
        decryptor = cipher.decryptor()

        padded_data = decryptor.update(ciphertext) + decryptor.finalize()

        unpadder = sym_padding.PKCS7(128).unpadder()
        data = unpadder.update(padded_data) + unpadder.finalize()

        return (data)

class PublicSecurityKey(SecurityKey):
    def __init__(self, key):
        super().__init__(key)

    @staticmethod
    def from_file(path):
        with open(path, "rb") as key_file:
            public_key = serialization.load_pem_public_key(
                key_file.read()
            )

        return (PublicSecurityKey(public_key))
    
    @staticmethod
    def from_string(string: str, /, encoding = "utf8"):
        public_key = serialization.load_pem_public_key(
            string.encode(encoding)
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

    def to_string(self, /, encoding = "utf8"):
        return (self.key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ).decode(encoding))

    # def encrypt(self, message: bytes):
    #     print(message)
    #     return self.key.encrypt(
    #         message,
    #         padding.OAEP(
    #             mgf=padding.MGF1(algorithm=hashes.SHA256()),
    #             algorithm=hashes.SHA256(),
    #             label=None
    #         )
    #     )

    def encrypt(self, message: bytes):
        aes_key = urandom(32)

        iv = urandom(16)

        cipher = Cipher(algorithms.AES(aes_key), modes.CBC(iv))
        encryptor = cipher.encryptor()

        padder = sym_padding.PKCS7(128).padder()
        padded_data = padder.update(message) + padder.finalize()

        ciphertext = encryptor.update(padded_data) + encryptor.finalize()

        encrypted_key = self.key.encrypt(
            aes_key,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )

        return (encrypted_key + iv + ciphertext)
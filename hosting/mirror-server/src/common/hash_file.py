from hashlib import sha256

def hash_file(file, algorithm = sha256, /, buffer_size = 65536):
    base_hash = algorithm()

    with open(file, 'rb') as fp:
        data = fp.read(buffer_size)

        while data:
            base_hash.update(data)
            data = fp.read(buffer_size)

    return base_hash.hexdigest()
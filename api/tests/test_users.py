from fastapi.testclient import TestClient
from api.main import app

client = TestClient(app)

def test_register_and_login():
    r = client.post("/api/v1/auth/register", json={
        "username": "grace",
        "email": "grace@example.com",
        "password": "securepass",
        "full_name": "Grace Hopper",
    })
    assert r.status_code == 201
    data = r.json()
    assert data["username"] == "grace"
    assert data["email"] == "grace@example.com"
    user_id = data["id"]

    r = client.post("/api/v1/auth/login", json={
        "email": "grace@example.com",
        "password": "securepass",
    })
    assert r.status_code == 200
    token = r.json()["access_token"]
    assert token

    r = client.get("/api/v1/users/me", headers={"Authorization": f"Bearer {token}"})
    assert r.status_code == 200
    body = r.json()
    assert body["username"] == "grace"
    assert body["id"] == user_id

def test_register_duplicate_email():
    client.post("/api/v1/auth/register", json={
        "username": "ada",
        "email": "ada@example.com",
        "password": "securepass",
    })
    r = client.post("/api/v1/auth/register", json={
        "username": "ada2",
        "email": "ada@example.com",
        "password": "securepass",
    })
    assert r.status_code == 409

def test_login_wrong_password():
    client.post("/api/v1/auth/register", json={
        "username": "turing",
        "email": "turing@example.com",
        "password": "securepass",
    })
    r = client.post("/api/v1/auth/login", json={
        "email": "turing@example.com",
        "password": "wrongpass",
    })
    assert r.status_code == 401

def test_read_user_by_id():
    r = client.post("/api/v1/auth/register", json={
        "username": "knuth",
        "email": "knuth@example.com",
        "password": "securepass",
    })
    user_id = r.json()["id"]

    r = client.get(f"/api/v1/users/{user_id}")
    assert r.status_code == 200
    assert r.json()["username"] == "knuth"

-- Test script for MiniSQL Interpreter
-- This script tests CREATE, INSERT, and SELECT operations

-- Create a users table
CREATE TABLE users (id, name, age);

-- Insert some test data
INSERT INTO users VALUES (1, Alice, 30);
INSERT INTO users VALUES (2, Bob, 25);
INSERT INTO users VALUES (3, Charlie, 35);

-- Select all users
SELECT * FROM users;

-- Select with WHERE clause
SELECT * FROM users WHERE age = 25;

-- Create another table
CREATE TABLE products (id, name, price);

-- Insert products
INSERT INTO products VALUES (101, "Laptop", 999);
INSERT INTO products VALUES (102, "Mouse", 25);
INSERT INTO products VALUES (103, "Keyboard", 75);

-- Select all products
SELECT * FROM products;

-- Select specific product
SELECT * FROM products WHERE name = Mouse;

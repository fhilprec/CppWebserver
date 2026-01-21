#!/bin/bash

# Script to generate self-signed SSL certificates for development/testing
# These certificates should NOT be used in production

echo "Generating self-signed SSL certificate for development..."

# Create certs directory if it doesn't exist
mkdir -p certs

# Generate private key and certificate in one command
openssl req -x509 -newkey rsa:4096 \
    -keyout certs/key.pem \
    -out certs/cert.pem \
    -days 365 \
    -nodes \
    -subj "/C=US/ST=State/L=City/O=Organization/OU=Development/CN=localhost"

if [ $? -eq 0 ]; then
    echo ""
    echo "Certificate and key generated successfully!"
    echo "  Certificate: certs/cert.pem"
    echo "  Private Key: certs/key.pem"
    echo ""
    echo "Note: These are self-signed certificates for development only."
    echo "Browsers will show security warnings when accessing https://localhost"
else
    echo "Failed to generate certificates"
    exit 1
fi

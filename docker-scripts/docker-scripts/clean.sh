#!/bin/bash
# Clean up Docker resources
echo "Cleaning up Docker resources..."
docker-compose down -v
docker system prune -f
echo "Cleanup completed"

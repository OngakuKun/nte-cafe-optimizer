# syntax=docker/dockerfile:1

FROM debian:stable-slim AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    patch \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
# WebUI embeds the server port in webui.js; behind a TLS reverse proxy the browser
# must use its own origin (https + public port), not the internal container port.
RUN patch -N -p1 -d lib/webui < patches/webui-proxy-bridge.patch || true
RUN cmake -S . -B /build -DCMAKE_BUILD_TYPE=Release -DWEBUI_BUILD_EXAMPLES=OFF \
 && cmake --build /build --target nte-cafe-app -j"$(nproc)"

FROM debian:stable-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -r -u 10001 -m appuser

WORKDIR /app
COPY --from=build /build/app/nte-cafe-app /app/nte-cafe-app
COPY --from=build /build/app/ui /app/ui
COPY gameversion.txt /app/ui/gameversion.txt

ENV PORT=80
EXPOSE 80 8080
USER appuser
CMD ["./nte-cafe-app"]

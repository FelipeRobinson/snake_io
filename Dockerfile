FROM alpine:3.22 AS build
RUN apk add --no-cache build-base
WORKDIR /app
COPY config.h game.h game.c server.c ./
COPY tests ./tests
RUN cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I. game.c tests/test_game.c -o test_game \
    && ./test_game \
    && cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic game.c server.c -o snake

FROM alpine:3.22
WORKDIR /app
COPY --from=build /app/snake ./snake
COPY static ./static
USER 10001:10001
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
    CMD wget -q -O /dev/null http://127.0.0.1:8000/health || exit 1
CMD ["./snake"]

FROM buildpack-deps:buster AS builder
WORKDIR /opt/bedtools
COPY . .
ARG MAKE_OPTS
RUN make static $MAKE_OPTS

FROM scratch
COPY --from=builder /opt/bedtools/bin/bedtools.static /
WORKDIR /data
ENTRYPOINT ["/bedtools.static"]

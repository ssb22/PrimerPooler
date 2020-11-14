# $FreeBSD$

PORTNAME=		pooler
DISTVERSIONPREFIX=	v
DISTVERSION=		1.74
CATEGORIES=		wip biology

MAINTAINER=	ssb22@cam.ac.uk
COMMENT=	PrimerPooler: optimise primer-set combinations

LICENSE=	APACHE20
LICENSE_FILE=	${WRKDIR}/${GH_PROJECT}-${PORTVERSION}/LICENSE

USE_GITHUB=	yes
GH_ACCOUNT=	ssb22
GH_PROJECT=	PrimerPooler

WRKSRC_SUBDIR=	pooler
INSTALL_TARGET=	install-strip

PLIST_FILES=	bin/pooler man/man1/pooler.1.gz

.include <bsd.port.mk>

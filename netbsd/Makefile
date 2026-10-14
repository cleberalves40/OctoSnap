# $NetBSD$

DISTNAME=	octosnap-0.1.0
CATEGORIES=	sysutils
MASTER_SITES=	${MASTER_SITE_GITHUB:=cleberalves40/}
GITHUB_PROJECT=	OctoSnap
GITHUB_TAG=	v${PKGVERSION_NOREV}

MAINTAINER=	cleberalves40@github.com
HOMEPAGE=	https://github.com/cleberalves40/OctoSnap
COMMENT=	Qt6 graphical front-end for NetBSD FFS snapshots (fss(4))
LICENSE=	modified-bsd

ONLY_FOR_PLATFORM=	NetBSD-*-*

USE_LANGUAGES=		c++
USE_CXX_FEATURES=	c++17

CMAKE_CONFIGURE_ARGS+=	-DCMAKE_PREFIX_PATH=${QTDIR}

.include "../../devel/cmake/build.mk"
.include "../../x11/qt6-qtbase/buildlink3.mk"
.include "../../devel/qt6-qttools/buildlink3.mk"
.include "../../mk/bsd.pkg.mk"

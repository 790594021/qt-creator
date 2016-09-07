/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <clangbackendipc_global.h>
#include <clangdocument.h>
#include <diagnosticcontainer.h>
#include <diagnosticset.h>
#include <fixitcontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocation.h>
#include <sourcelocationcontainer.h>
#include <sourcerangecontainer.h>
#include <clangtranslationunitcore.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "matcher-diagnosticcontainer.h"
#include "gtest-qt-printing.h"

using ::testing::Contains;
using ::testing::Not;
using ::testing::PrintToString;

using ::ClangBackEnd::Diagnostic;
using ::ClangBackEnd::DiagnosticSet;
using ::ClangBackEnd::DiagnosticContainer;
using ::ClangBackEnd::FixItContainer;
using ::ClangBackEnd::ProjectPart;
using ::ClangBackEnd::SourceLocation;
using ::ClangBackEnd::SourceLocationContainer;
using ::ClangBackEnd::Document;
using ::ClangBackEnd::UnsavedFiles;

namespace {

const Utf8String headerFilePath = Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnosticset_header.cpp");

class DiagnosticSet : public ::testing::Test
{
protected:
    ProjectPart projectPart{Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-pedantic")}};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnosticset.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents};
    Document documentMainFile{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_diagnosticset_mainfile.cpp"),
                              projectPart,
                              Utf8StringVector(),
                              documents};

protected:
    enum ChildMode { WithChild, WithoutChild };
    DiagnosticContainer expectedDiagnostic(ChildMode childMode) const;
};

TEST_F(DiagnosticSet, SetHasContent)
{
    document.parse();
    const auto set = document.translationUnitCore().diagnostics();

    ASSERT_THAT(set.size(), 1);
}

TEST_F(DiagnosticSet, MoveConstructor)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    const auto set2 = std::move(set);

    ASSERT_TRUE(set.isNull());
    ASSERT_FALSE(set2.isNull());
}

TEST_F(DiagnosticSet, MoveAssigment)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    auto set2 = std::move(set);
    set = std::move(set2);

    ASSERT_TRUE(set2.isNull());
    ASSERT_FALSE(set.isNull());
}

TEST_F(DiagnosticSet, MoveSelfAssigment)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    set = std::move(set);

    ASSERT_FALSE(set.isNull());
}

TEST_F(DiagnosticSet, FirstElementEqualBegin)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    ASSERT_TRUE(set.front() == *set.begin());
}

TEST_F(DiagnosticSet, BeginIsUnequalEnd)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    ASSERT_TRUE(set.begin() != set.end());
}

TEST_F(DiagnosticSet, BeginPlusOneIsEqualEnd)
{
    document.parse();
    auto set = document.translationUnitCore().diagnostics();

    ASSERT_TRUE(++set.begin() == set.end());
}

TEST_F(DiagnosticSet, ToDiagnosticContainersLetThroughByDefault)
{
    const auto diagnosticContainerWithoutChild = expectedDiagnostic(WithChild);
    documentMainFile.parse();

    const auto diagnostics = documentMainFile.translationUnitCore().diagnostics().toDiagnosticContainers();

    ASSERT_THAT(diagnostics, Contains(IsDiagnosticContainer(diagnosticContainerWithoutChild)));
}

TEST_F(DiagnosticSet, ToDiagnosticContainersFiltersOutTopLevelItem)
{
    documentMainFile.parse();
    const ::DiagnosticSet diagnosticSetWithChildren{documentMainFile.translationUnitCore().diagnostics()};
    const auto acceptNoDiagnostics = [](const Diagnostic &) { return false; };

    const auto diagnostics = diagnosticSetWithChildren.toDiagnosticContainers(acceptNoDiagnostics);

    ASSERT_TRUE(diagnostics.isEmpty());
}

DiagnosticContainer DiagnosticSet::expectedDiagnostic(DiagnosticSet::ChildMode childMode) const
{
    QVector<DiagnosticContainer> children;
    if (childMode == WithChild) {
        const auto child = DiagnosticContainer(
            Utf8StringLiteral("note: previous definition is here"),
            Utf8StringLiteral("Semantic Issue"),
            {Utf8String(), Utf8String()},
            ClangBackEnd::DiagnosticSeverity::Note,
            SourceLocationContainer(headerFilePath, 1, 6),
            {},
            {},
            {}
        );
        children.append(child);
    }

    return DiagnosticContainer(
        Utf8StringLiteral("error: redefinition of 'f'"),
        Utf8StringLiteral("Semantic Issue"),
        {Utf8String(), Utf8String()},
        ClangBackEnd::DiagnosticSeverity::Error,
        SourceLocationContainer(documentMainFile.filePath(), 3, 6),
        {},
        {},
        children
    );
}

}

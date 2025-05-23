/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/
#include "UnitTestPCH.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include "TestModelFactory.h"

using namespace Assimp;

class utIssues : public ::testing::Test {};

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F( utIssues, OpacityBugWhenExporting_727 ) {
    float opacity;
    aiScene *scene = TestModelFactory::createDefaultTestModel(opacity);
    Assimp::Importer importer;
    Assimp::Exporter exporter;

    const aiExportFormatDesc *desc = exporter.GetExportFormatDescription( 0 );
    ASSERT_NE( desc, nullptr );

    std::string path = "dae";
    path.append(".");
    path.append( desc->fileExtension );
    EXPECT_EQ( AI_SUCCESS, exporter.Export( scene, desc->id, path ) );
    const aiScene *newScene( importer.ReadFile( path, aiProcess_ValidateDataStructure ) );
    ASSERT_NE( nullptr, newScene );
    float newOpacity;
    if (newScene->mNumMaterials > 0 ) {
        EXPECT_EQ( AI_SUCCESS, newScene->mMaterials[ 0 ]->Get( AI_MATKEY_OPACITY, newOpacity ) );
        EXPECT_FLOAT_EQ( opacity, newOpacity );
    }
    
    TestModelFactory::releaseDefaultTestModel(&scene);

    // Cleanup. Delete exported dae.dae file
    EXPECT_EQ(0, std::remove(path.c_str()));
}

#endif // ASSIMP_BUILD_NO_EXPORT

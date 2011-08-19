// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualSkeletonLoader.hpp"
#include <OgreSkeleton.h>
#include <OgreBone.h>
#include <OgreAnimation.h>

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Mesh;

ManualSkeletonLoader::ManualSkeletonLoader(MeshdataPtr meshdata, const std::set<String>& animationList)
 : mdptr(meshdata), skeletonLoaded(false), animations(animationList)
{
}

double ManualSkeletonLoader::invert(Matrix4x4f& inv, const Matrix4x4f& orig)
{
    float mat[16];
    float dst[16];

    int counter = 0;
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            mat[counter] = orig(i,j);
            counter++;
        }
    }

    float tmp[12]; /* temp array for pairs */
    float src[16]; /* array of transpose source matrix */
    float det; /* determinant */
    /* transpose matrix */
    for (int i = 0; i < 4; i++) {
        src[i] = mat[i*4];
        src[i + 4] = mat[i*4 + 1];
        src[i + 8] = mat[i*4 + 2];
        src[i + 12] = mat[i*4 + 3];
    }
    /* calculate pairs for first 8 elements (cofactors) */
    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];
    /* calculate first 8 elements (cofactors) */
    dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
    dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
    dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
    dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
    dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
    dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
    dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
    dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
    dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
    dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
    dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
    dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
    dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
    dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
    dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
    dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
    /* calculate pairs for second 8 elements (cofactors) */
    tmp[0] = src[2]*src[7];
    tmp[1] = src[3]*src[6];
    tmp[2] = src[1]*src[7];
    tmp[3] = src[3]*src[5];
    tmp[4] = src[1]*src[6];
    tmp[5] = src[2]*src[5];
    tmp[6] = src[0]*src[7];
    tmp[7] = src[3]*src[4];
    tmp[8] = src[0]*src[6];
    tmp[9] = src[2]*src[4];
    tmp[10] = src[0]*src[5];
    tmp[11] = src[1]*src[4];
    /* calculate second 8 elements (cofactors) */
    dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
    dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
    dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
    dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
    dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
    dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
    dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
    dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
    dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
    dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
    dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
    dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
    dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
    dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
    dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
    dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
    /* calculate determinant */
    det=src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];

    if (det == 0.0) return 0.0;

    /* calculate matrix inverse */
    det = 1/det;
    for (int j = 0; j < 16; j++)
        dst[j] *= det;

    counter = 0;
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            inv(i,j) = dst[counter];
            counter++;
        }
    }

    return det;
}

bool ManualSkeletonLoader::getTRS(const Matrix4x4f& bsm, Ogre::Vector3& translate, Ogre::Quaternion& quaternion, Ogre::Vector3& scale) {
    Vector4f trans = bsm.getCol(3);

    // Get the scaling matrix
    float32 scaleX = bsm.getCol(0).length();
    float32 scaleY = bsm.getCol(1).length();
    float32 scaleZ = bsm.getCol(2).length();

    // Get the rotation quaternion
    Vector4f xrot =  bsm.getCol(0)/scaleX;
    Vector4f yrot =  bsm.getCol(1)/scaleY;
    Vector4f zrot =  bsm.getCol(2)/scaleY;
    Matrix4x4f rotmat(xrot, yrot, zrot, Vector4f(0,0,0,1), Matrix4x4f::COLUMNS());

    float32 trace = rotmat(0,0) + rotmat(1,1) + rotmat(2,2) ;

    float32 qw,qx,qy,qz;
    if (trace > 0) {
        float32 S = sqrt(trace + 1);

        qw =  0.5f * S ;
        S = 0.5f / S;
        qx = (rotmat(2,1)-rotmat(1,2)) * S;
        qy = (rotmat(0,2)-rotmat(2,0)) * S ;
        qz = (rotmat(1,0)-rotmat(0,1)) * S;
    }
    else {
        //code in this block copied from Ogre Quaternion...

        static size_t s_iNext[3] = { 1, 2, 0 };
        size_t i = 0;
        if ( rotmat(1,1) > rotmat(0,0) )
            i = 1;
        if ( rotmat(2,2) > rotmat(i,i) )
            i = 2;
        size_t j = s_iNext[i];
        size_t k = s_iNext[j];

        float32 fRoot = sqrt(rotmat(i,i)-rotmat(j,j)-rotmat(k,k) + 1.0f);
        float32* apkQuat[3] = { &qx, &qy, &qz };
        *apkQuat[i] = 0.5f*fRoot;
        fRoot = 0.5f/fRoot;
        qw = (rotmat(k,j)-rotmat(j,k))*fRoot;
        *apkQuat[j] = (rotmat(j,i)+rotmat(i,j))*fRoot;
        *apkQuat[k] = (rotmat(k,i)+rotmat(i,k))*fRoot;
    }

    float32 N = sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
    qx /= N; qy /= N; qz /= N; qw /= N;

    Matrix4x4f scalemat( Matrix4x4f::identity()  );
    scalemat(0,0) = scaleX;
    scalemat(1,1) = scaleY;
    scalemat(2,2) = scaleZ;

    Matrix4x4f transmat(Matrix4x4f::identity());
    transmat(0,3) = trans.x;
    transmat(1,3) = trans.y;
    transmat(2,3) = trans.z;

    Matrix4x4f diffmat = (bsm - (transmat*rotmat*scalemat));
    float32 matlen = diffmat.getCol(0).length() + diffmat.getCol(1).length() + diffmat.getCol(2).length()
        + diffmat.getCol(3).length() ;

    if (matlen > 0.00001) {
        return false;
    }

    quaternion = Ogre::Quaternion(qw, qx, qy, qz);

    translate = Ogre::Vector3(trans.x, trans.y, trans.z);

    scale = Ogre::Vector3( scaleX, scaleY, scaleZ);

    return true;
}

void ManualSkeletonLoader::loadResource(Ogre::Resource *r) {
    Ogre::Skeleton* skel = dynamic_cast<Ogre::Skeleton*> (r);

    // We have to set some binding information for each joint (bind shape
    // matrix * inverseBindMatrix_i for each joint i). Collada has a more
    // flexible animation system, so here we only support this if there is 1
    // SubMeshGeometry with 1 SkinController
    if (mdptr->geometry.size() != 1 || mdptr->geometry[0].skinControllers.size() != 1) {
        SILOG(ogre,error,"Only know how to handle skinning for 1 SubMeshGeometry and 1 SkinController. Leaving skeleton unbound.");
        return;
    }
    const SkinController& skin = mdptr->geometry[0].skinControllers[0];

    typedef std::map<uint32, Ogre::Bone*> BoneMap;
    BoneMap bones;

    Ogre::Vector3 translate(0,0,0), scale(1,1,1);
    Ogre::Quaternion rotate(1,0,0,0);
    Meshdata::JointIterator joint_it = mdptr->getJointIterator();
    uint32 joint_id;
    uint32 joint_idx;
    Matrix4x4f pos_xform;
    uint32 parent_id;
    std::vector<Matrix4x4f> transformList;

    // We build animations as we go since the Meshdata doesn't
    // have a central index of animations -- we need to add them
    // as we encounter them and then adjust them, adding tracks
    // for different bones, as we go.
    typedef std::map<String, Ogre::Animation*> AnimationMap;
    AnimationMap animationMap;

    // Bones
    // Basic root bone, no xform

    Matrix4x4f bsm = skin.bindShapeMatrix;
    Matrix4x4f B_inv;
    invert(B_inv, bsm);

    bones[0] = skel->createBone(0);

    unsigned short curBoneValue = mdptr->getJointCount()+1;

    std::tr1::unordered_map<uint32, Matrix4x4f> ibmMap;

    while( joint_it.next(&joint_id, &joint_idx, &pos_xform, &parent_id, transformList) ) {

        // We need to work backwards from the joint_idx (index into
        // mdptr->joints) to the index of the joint in this skin controller
        // (so we can lookup inverseBindMatrices).
        uint32 skin_joint_idx = 0;
        for(skin_joint_idx = 0; skin_joint_idx < skin.joints.size(); skin_joint_idx++) {
            if (skin.joints[skin_joint_idx] == joint_idx) break;
        }

        Matrix4x4f ibm = Matrix4x4f::identity();
        if (skin_joint_idx < skin.joints.size()) {
            // Get the bone's inverse bind-pose matrix and store it in the ibmMap.
            ibm = skin.inverseBindMatrices[skin_joint_idx];
        }

        ibmMap[joint_id] = ibm;

        /* Now construct the bone hierarchy. First implement the transform hierarchy from root to the bone's node. */
        Ogre::Bone* bone = bones[parent_id];

        if (bone == NULL) {
            SILOG(ogre, error, "Could not load skeleton for this mesh. This format is not currently supported");
            return;
        }

        if (parent_id == 0) {
            for (uint32 i = 0;   i < transformList.size() ; i++) {
                Matrix4x4f mat = transformList[i];

                bool ret = getTRS(mat, translate, rotate, scale);
                assert(ret);

                bone = bone->createChild(curBoneValue++, translate, rotate);
                bone->setScale(scale);
            }
        }

        const Node& node = mdptr->nodes[ mdptr->joints[joint_idx] ];

        /* Finally create the actual bone */

        bone = bone->createChild(joint_id);

        bones[joint_id] = bone;

        for (std::set<String>::const_iterator anim_it = animations.begin(); anim_it != animations.end(); anim_it++) {
            // Find/create the animation
            const String& anim_name = *anim_it;

            if (animationMap.find(anim_name) == animationMap.end())
                animationMap[anim_name] = skel->createAnimation(anim_name, 0.0);
            Ogre::Animation* anim = animationMap[anim_name];

            Ogre::Vector3 startPos(0,0,0);
            Ogre::Quaternion startOrientation(1,0,0,0);
            Ogre::Vector3 startScale(1,1,1);

            // If this node has associated animation keyframes with this name, then use those keyframes.
            if (node.animations.find(anim_name) != node.animations.end()) {
                const TransformationKeyFrames& anim_key_frames = node.animations.find(anim_name)->second;

                // Add track, making sure we set the length long
                // enough to support this new track

                Ogre::NodeAnimationTrack* track = anim->createNodeTrack(joint_id, bone);

                for(uint32 key_it = 0; key_it < anim_key_frames.inputs.size() ; key_it++) {
                    float32 key_time = anim_key_frames.inputs[key_it];

                    anim->setLength( std::max(anim->getLength(), key_time) );
                    Matrix4x4f mat =  anim_key_frames.outputs[key_it] * ibm * bsm;

                    if (parent_id != 0) {
                        //need to cancel out the effect of BSM*IBM from the parent in the hierarchy
                        Matrix4x4f parentI_inv;
                        invert(parentI_inv, ibmMap[parent_id]);

                        mat = B_inv * parentI_inv * mat;
                    }

                    bool ret = getTRS(mat, translate, rotate, scale);
                    assert(ret);

                    if (ret ) {
                        //Set the transform for the keyframe.
                        Ogre::TransformKeyFrame* key = track->createNodeKeyFrame(key_time);

                        key->setTranslate( translate - startPos );

                        Ogre::Quaternion quat = startOrientation.Inverse() *  rotate;

                        key->setRotation( quat );
                        key->setScale( scale );
                    }
                }
            }
            else { //otherwise, just create a single keyframe using the node's transform.
                Ogre::NodeAnimationTrack* track = anim->createNodeTrack(joint_id, bone);

                float32 key_time = 0;

                anim->setLength( std::max(anim->getLength(), key_time) );
                Matrix4x4f mat =  node.transform * ibm * bsm;

                if (parent_id != 0) {
                    //need to cancel out the effect of BSM*IBM from the parent in the hierarchy
                    Matrix4x4f parentI_inv;
                    invert(parentI_inv, ibmMap[parent_id]);

                    mat = B_inv * parentI_inv * mat;
                }

                bool ret = getTRS(mat, translate, rotate, scale);
                assert(ret);

                if (ret ) {
                    //Set the transform for the keyframe.
                    Ogre::TransformKeyFrame* key = track->createNodeKeyFrame(key_time);

                    key->setTranslate( translate - startPos );

                    Ogre::Quaternion quat = startOrientation.Inverse() *  rotate;

                    key->setRotation( quat );
                    key->setScale( scale );
                }
            }
        }
    }

    skeletonLoaded = true;
}

} // namespace Graphics
} // namespace Sirikata

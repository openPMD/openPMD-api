
#include "openPMD/IO/InvalidatableFile.hpp"


openPMD::InvalidatableFile::InvalidatableFile( std::string s ) :
    fileState { std::make_shared< FileState >( s ) }
{}


void openPMD::InvalidatableFile::invalidate( )
{
    fileState->valid = false;
}


bool openPMD::InvalidatableFile::valid( ) const
{
    return fileState->valid;
}


openPMD::InvalidatableFile &
openPMD::InvalidatableFile::operator=( std::string s )
{
    if( fileState )
    {
        fileState->name = s;
    }
    else
    {
        fileState = std::make_shared< FileState >( s );
    }
    return *this;
}


bool
openPMD::InvalidatableFile::operator==( const openPMD::InvalidatableFile & f ) const
{
    return this->fileState == f.fileState;
}


std::string & openPMD::InvalidatableFile::operator*( ) const
{
    return fileState->name;
}


std::string * openPMD::InvalidatableFile::operator->( ) const
{
    return &fileState->name;
}


openPMD::InvalidatableFile::operator bool( ) const
{
    return fileState.operator bool( );
}


openPMD::InvalidatableFile::FileState::FileState( std::string s ) :
    name { std::move( s ) }
{}

std::hash< openPMD::InvalidatableFile >::result_type
std::hash< openPMD::InvalidatableFile >::operator()( const openPMD::InvalidatableFile & s ) const noexcept
{
    return std::hash< shared_ptr< openPMD::InvalidatableFile::FileState>> {}( s.fileState );
}

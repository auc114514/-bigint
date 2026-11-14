#include <bits/stdc++.h>
using namespace std;
class bigint{
public:
    using limb_t=uint64_t;
    using mag_t=vector<limb_t>;
    using view_t=span<const limb_t>;
    static constexpr size_t KARATSUBA_THRESHOLD=32;
    static constexpr size_t SMALL_LIMB_THRESHOLD=4;
    static constexpr limb_t BASE10_19=10'000'000'000'000'000'000ULL;
private:
    bool neg{false};
    mag_t mag;
    static_assert(is_same_v<limb_t,uint64_t>,"limb_t must be uint64_t");
    static_assert(sizeof(limb_t)*8==64,"limb must be 64 bits");
    static_assert(BASE10_19<=numeric_limits<limb_t>::max(),"10^19 must fit in 64 bits");
    void normalize(){
        while(!mag.empty()&&mag.back()==0)mag.pop_back();
        if(mag.empty())neg=false;
        assert(mag.empty()||mag.back()!=0);
    }
    static view_t trimmed(view_t v){
        size_t k=v.size();
        while(k&&v[k-1]==0)--k;
        return v.first(k);
    }
    static bool mag_lt(view_t a,view_t b){
        if(a.size()!=b.size())return a.size()<b.size();
        for(size_t i=a.size();i-->0;)if(a[i]!=b[i])return a[i]<b[i];
        return false;
    }
    static bool mag_lt(const mag_t&a,const mag_t&b){
        return mag_lt(view_t(a),view_t(b));
    }
    static void mag_add_view(view_t a,view_t b,mag_t&out){
        size_t na=a.size(),nb=b.size();
        out.assign(max(na,nb),0);
        limb_t carry=0;
        size_t i=0;
        size_t m=min(na,nb);
        for(;i<m;++i){
            limb_t s=a[i]+b[i];
            limb_t c1=(s<a[i]);
            limb_t s2=s+carry;
            limb_t c2=(s2<s);
            out[i]=s2;
            carry=c1|c2;
        }
        for(;i<na;++i){
            limb_t s=a[i]+carry;
            carry=(s<a[i]);
            out[i]=s;
        }
        for(;i<nb;++i){
            limb_t s=b[i]+carry;
            carry=(s<b[i]);
            out[i]=s;
        }
        if(carry)out.push_back(carry);
    }
    static void mag_add(mag_t&a,const mag_t&b){
        mag_t tmp;
        mag_add_view(view_t(a),view_t(b),tmp);
        a=move(tmp);
    }
    static void mag_sub_view(mag_t&a,view_t b){
        assert(!mag_lt(view_t(a),b)&&"mag_sub: |a| >= |b| violated");
        limb_t borrow=0;
        for(size_t i=0;i<b.size();++i){
            limb_t ai=a[i],bi=b[i];
            limb_t d1=ai-bi;
            limb_t b1=(ai<bi);
            limb_t d2=d1-borrow;
            limb_t b2=(d1<borrow);
            a[i]=d2;
            borrow=b1|b2;
        }
        for(size_t i=b.size();borrow&&i<a.size();++i){
            limb_t d=a[i]-borrow;
            borrow=(a[i]<borrow);
            a[i]=d;
        }
        while(!a.empty()&&a.back()==0)a.pop_back();
    }
    static void mag_sub(mag_t&a,const mag_t&b){mag_sub_view(a,view_t(b));}
    static void mag_mul_limb_into(view_t a,limb_t b,mag_t&out){
        if(a.empty()||b==0){out.clear();return;}
        out.assign(a.size(),0);
        unsigned __int128 carry=0;
        for(size_t i=0;i<a.size();++i){
            unsigned __int128 cur=(unsigned __int128)a[i]*b+carry;
            out[i]=(limb_t)cur;
            carry=cur>>64;
        }
        if(carry)out.push_back((limb_t)carry);
    }
    static void mag_mul_into_4way(view_t a,view_t b,mag_t&out){
        size_t na=a.size(),nb=b.size();
        if(!na||!nb){out.clear();return;}
        out.assign(na+nb,0);
        for(size_t i=0;i<na;i+=4){
            size_t m=min<size_t>(4,na-i);
            limb_t carry[4]={0,0,0,0};
            for(size_t j=0;j<nb;++j){
                for(size_t k=0;k<m;++k){
                    unsigned __int128 cur=
                        (unsigned __int128)a[i+k]*b[j]+out[i+k+j]+carry[k];
                    out[i+k+j]=(limb_t)cur;
                    carry[k]=(limb_t)(cur>>64);
                }
            }
            for(size_t k=0;k<m;++k)
                out[i+k+nb]=carry[k];
        }
        while(!out.empty()&&out.back()==0)out.pop_back();
    }
    struct Scratch{
        deque<mag_t>bufs;
        size_t top=0;
        explicit Scratch(size_t reserve=16){
            for(size_t i=0;i<reserve;++i)bufs.emplace_back();
        }
        mag_t& get(){
            if(top==bufs.size())bufs.emplace_back();
            return bufs[top++];
        }
        void release(size_t n){top-=n;}
    };
    static void add_at(mag_t&out,const mag_t&z,size_t off){
        size_t i=0;
        unsigned __int128 carry=0;
        for(;i<z.size();++i){
            unsigned __int128 cur=(unsigned __int128)out[off+i]+z[i]+carry;
            out[off+i]=(limb_t)cur;
            carry=cur>>64;
        }
        size_t k=off+i;
        while(carry){
            unsigned __int128 cur=(unsigned __int128)out[k]+carry;
            out[k]=(limb_t)cur;
            carry=cur>>64;
            ++k;
        }
    }
    static void mag_mul_karatsuba_v(view_t a,view_t b,mag_t&out,Scratch&sc){
        a=trimmed(a);b=trimmed(b);
        size_t n=max(a.size(),b.size());
        if(n<KARATSUBA_THRESHOLD){mag_mul_into_4way(a,b,out);return;}
        size_t half=n/2;
        size_t sa=min(half,a.size());
        size_t sb=min(half,b.size());
        view_t a0=a.first(sa);
        view_t a1=a.subspan(sa);
        view_t b0=b.first(sb);
        view_t b1=b.subspan(sb);
        mag_t&z0=sc.get();
        mag_t&z2=sc.get();
        mag_t&z1=sc.get();
        mag_t&sav=sc.get();
        mag_t&sbv=sc.get();
        mag_mul_karatsuba_v(a0,b0,z0,sc);
        mag_mul_karatsuba_v(a1,b1,z2,sc);
        mag_add_view(a0,a1,sav);
        mag_add_view(b0,b1,sbv);
        mag_mul_karatsuba_v(view_t(sav),view_t(sbv),z1,sc);
        mag_sub(z1,z0);
        mag_sub(z1,z2);
        out.assign(a.size()+b.size(),0);
        add_at(out,z0,0);
        add_at(out,z1,half);
        add_at(out,z2,2*half);
        sc.release(5);
        while(!out.empty()&&out.back()==0)out.pop_back();
    }
    static void mag_sqr_into(view_t a,mag_t&out){
        size_t n=a.size();
        if(!n){out.clear();return;}
        out.assign(2*n,0);
        for(size_t i=0;i<n;++i){
            unsigned __int128 carry=0;
            for(size_t j=i+1;j<n;++j){
                unsigned __int128 cur=
                    (unsigned __int128)a[i]*a[j]+out[i+j]+carry;
                out[i+j]=(limb_t)cur;
                carry=cur>>64;
            }
            out[i+n]=(limb_t)carry;
        }
        limb_t carry=0;
        for(size_t k=0;k<2*n;++k){
            limb_t v=out[k];
            out[k]=(v<<1)|carry;
            carry=v>>63;
        }
        for(size_t i=0;i<n;++i){
            unsigned __int128 cur=(unsigned __int128)a[i]*a[i]+out[2*i];
            out[2*i]=(limb_t)cur;
            unsigned __int128 c=cur>>64;
            for(size_t k=2*i+1;c;++k){
                assert(k<out.size()&&"mag_sqr_into: carry overflow");
                unsigned __int128 s=(unsigned __int128)out[k]+c;
                out[k]=(limb_t)s;
                c=s>>64;
            }
        }
        while(!out.empty()&&out.back()==0)out.pop_back();
    }
    static void mag_shl(mag_t&a,size_t k){
        if(a.empty()||k==0)return;
        size_t ls=k/64,bs=k%64;
        if(ls)a.insert(a.begin(),ls,0);
        if(bs){
            limb_t carry=0;
            for(size_t i=ls;i<a.size();++i){
                limb_t v=a[i];
                a[i]=(v<<bs)|carry;
                carry=v>>(64-bs);
            }
            if(carry)a.push_back(carry);
        }
    }
    static void mag_shr(mag_t&a,size_t k){
        if(a.empty()||k==0)return;
        size_t ls=k/64,bs=k%64;
        if(ls>=a.size()){a.clear();return;}
        a.erase(a.begin(),a.begin()+ls);
        if(bs){
            limb_t carry=0;
            for(size_t i=a.size();i-->0;){
                limb_t v=a[i];
                a[i]=(v>>bs)|carry;
                carry=v<<(64-bs);
            }
        }
        while(!a.empty()&&a.back()==0)a.pop_back();
    }
    static void mag_divmod(view_t u_in,view_t v_in,mag_t&q,mag_t&r){
        q.clear();r.clear();
        if(v_in.empty())throw domain_error("bigint: divide by zero");
        if(mag_lt(u_in,v_in)){r.assign(u_in.begin(),u_in.end());return;}
        if(v_in.size()==1){
            limb_t d=v_in[0];
            q.assign(u_in.size(),0);
            limb_t rem=0;
            for(size_t i=u_in.size();i-->0;){
                unsigned __int128 cur=((unsigned __int128)rem<<64)|u_in[i];
                q[i]=(limb_t)(cur/d);
                rem=(limb_t)(cur%d);
            }
            while(!q.empty()&&q.back()==0)q.pop_back();
            if(rem)r.push_back(rem);
            return;
        }
        size_t n=v_in.size(),m=u_in.size();
        int shift=__builtin_clzll(v_in[n-1]);
        vector<limb_t>u(m+1),v(n);
        if(shift==0){
            copy_n(u_in.begin(),m,u.begin());
            u[m]=0;
        }else{
            limb_t carry=0;
            for(size_t i=0;i<m;++i){
                u[i]=(u_in[i]<<shift)|carry;
                carry=u_in[i]>>(64-shift);
            }
            u[m]=carry;
        }
        {
            limb_t carry=0;
            for(size_t i=0;i<n;++i){
                v[i]=(v_in[i]<<shift)|carry;
                carry=v_in[i]>>(64-shift);
            }
        }
        q.assign(m-n+1,0);
        for(size_t j=m-n+1;j-->0;){
            unsigned __int128 num=((unsigned __int128)u[j+n]<<64)|u[j+n-1];
            limb_t qhat=(limb_t)(num/v[n-1]);
            limb_t rhat=(limb_t)(num%v[n-1]);
            while(qhat==(limb_t(1)<<63)||
                   (unsigned __int128)qhat*v[n-2]>
                   (((unsigned __int128)rhat<<64)|u[j+n-2])){
                --qhat;
                rhat+=v[n-1];
                if(rhat<v[n-1])break;
            }
            uint64_t borrow=0;
            unsigned __int128 carry_m=0;
            for(size_t i=0;i<n;++i){
                unsigned __int128 prod=(unsigned __int128)qhat*v[i]+carry_m;
                carry_m=prod>>64;
                limb_t sub=(limb_t)prod;
                unsigned __int128 t=(unsigned __int128)u[j+i]-sub-borrow;
                u[j+i]=(limb_t)t;
                borrow=(uint64_t)(t>>64)&1ULL;
            }
            unsigned __int128 t=(unsigned __int128)u[j+n]-carry_m-borrow;
            u[j+n]=(limb_t)t;
            if((t>>64)!=0){
                --qhat;
                limb_t c=0;
                for(size_t i=0;i<n;++i){
                    unsigned __int128 s=(unsigned __int128)u[j+i]+v[i]+c;
                    u[j+i]=(limb_t)s;
                    c=(limb_t)(s>>64);
                }
                u[j+n]+=c;
            }
            q[j]=qhat;
        }
        while(!q.empty()&&q.back()==0)q.pop_back();
        r.assign(n,0);
        if(shift==0){
            copy_n(u.begin(),n,r.begin());
        }else{
            limb_t carry=0;
            for(size_t i=n;i-->0;){
                r[i]=(u[i]>>shift)|carry;
                carry=u[i]<<(64-shift);
            }
        }
        while(!r.empty()&&r.back()==0)r.pop_back();
    }
    static bigint from_limb(limb_t v){
        bigint r;
        if(v)r.mag.push_back(v);
        return r;
    }
    static bigint from_dec_str(const string&s){
        bigint r;
        size_t i=0;
        bool n=false;
        if(i<s.size()&&(s[i]=='-'||s[i]=='+')){
            n=(s[i]=='-');
            ++i;
        }
        if(i==s.size())
            throw invalid_argument("bigint: no digits in decimal string");
        for(;i<s.size();){
            size_t end=min(i+19,s.size());
            limb_t chunk=0,mul=1;
            for(size_t k=i;k<end;++k){
                if(!isdigit(static_cast<unsigned char>(s[k])))
                    throw invalid_argument("bigint: bad decimal");
                chunk=chunk*10+(s[k]-'0');
                mul*=10;
            }
            mag_t tmp;
            mag_mul_limb_into(view_t(r.mag),mul,tmp);
            limb_t carry=chunk;
            for(size_t k=0;carry&&k<tmp.size();++k){
                limb_t s2=tmp[k]+carry;
                carry=(s2<tmp[k]);
                tmp[k]=s2;
            }
            if(carry)tmp.push_back(carry);
            r.mag=move(tmp);
            r.normalize();
            i=end;
        }
        if(!r.is_zero()&&n)r.neg=true;
        return r;
    }
    static mag_t to_twos(const bigint&x,size_t W){
        mag_t m=x.mag;
        m.resize(W,0);
        if(x.neg){
            for(auto&v:m)v=~v;
            limb_t carry=1;
            for(size_t i=0;i<W;++i){
                limb_t s=m[i]+carry;
                carry=(s<m[i]);
                m[i]=s;
            }
        }
        return m;
    }
    static bigint from_twos(mag_t m){
        bigint r;
        bool n=(m.back()>>63)&1;
        if(n){
            for(auto&v:m)v=~v;
            limb_t carry=1;
            for(size_t i=0;i<m.size();++i){
                limb_t s=m[i]+carry;
                carry=(s<m[i]);
                m[i]=s;
            }
        }
        r.mag=move(m);
        r.neg=n;
        r.normalize();
        return r;
    }
    size_t work_width(size_t other_size=0)const{
        return max(mag.size(),other_size)+1;
    }
    static void check_width_runtime(const bigint&a,const bigint&b,size_t W){
        size_t need=max(a.mag.size(),b.mag.size())+1;
        if(W<need)
            throw invalid_argument(
                "bigint: width too small (need >= "+std::to_string(need)+")");
    }
public:
    bigint()=default;
    bigint(long long v):neg(v<0){
        if(v<0)mag.push_back(limb_t(0)-limb_t(v));
        else if(v)mag.push_back(limb_t(v));
        normalize();
    }
    bigint(const bigint&)=default;
    bigint& operator=(const bigint&)=default;
    bigint(bigint&&)noexcept=default;
    bigint& operator=(bigint&&)noexcept=default;
    void swap(bigint&o)noexcept{std::swap(neg,o.neg);mag.swap(o.mag);}
    bool is_zero()const noexcept{return mag.empty();}
    bool is_odd()const noexcept{return!mag.empty()&&(mag[0]&1);}
    static bigint from_hex(const string&s){
        bigint r;
        size_t i=0;
        bool n=false;
        if(i<s.size()&&(s[i]=='-'||s[i]=='+')){
            n=(s[i]=='-');++i;
        }
        if(i+1<s.size()&&s[i]=='0'&&(s[i+1]=='x'||s[i+1]=='X'))
            i+=2;
        if(i==s.size())throw invalid_argument("bigint: empty hex");
        size_t rem=s.size()-i;
        size_t limbs=(rem+15)/16;
        r.mag.assign(limbs,0);
        for(size_t k=0;k<rem;++k){
            char c=s[s.size()-1-k];
            int d;
            if(c>='0'&&c<='9')d=c-'0';
            else if(c>='a'&&c<='f')d=c-'a'+10;
            else if(c>='A'&&c<='F')d=c-'A'+10;
            else throw invalid_argument("bigint: bad hex");
            r.mag[k/16]|=limb_t(d)<<((k%16)*4);
        }
        r.normalize();
        if(!r.is_zero()&&n)r.neg=true;
        return r;
    }
    static bigint from_oct_str(const string&s){
        bigint r;
        size_t i=0;
        bool n=false;
        if(i<s.size()&&(s[i]=='-'||s[i]=='+')){
            n=(s[i]=='-');
            ++i;
        }
        if(i+1<s.size()&&s[i]=='0'&&(s[i+1]=='o'||s[i+1]=='O'))
            i+=2;
        if(i==s.size())
            throw invalid_argument("bigint: no digits in octal string");
        for(;i<s.size();++i){
            char c=s[i];
            if(c<'0'||c>'7')
                throw invalid_argument("bigint: bad octal digit");
            mag_t tmp;
            mag_mul_limb_into(view_t(r.mag),8,tmp);
            limb_t carry=c-'0';
            for(size_t k=0;carry&&k<tmp.size();++k){
                limb_t s2=tmp[k]+carry;
                carry=(s2<tmp[k]);
                tmp[k]=s2;
            }
            if(carry)tmp.push_back(carry);
            r.mag=move(tmp);
            r.normalize();
        }
        if(!r.is_zero()&&n)r.neg=true;
        return r;
    }
    static bigint from_string(const string&s){return from_dec_str(s);}
    friend istream& operator>>(istream&is,bigint&v){
        string s;
        if(is>>s){
            if(s.empty()){is.setstate(ios::failbit);return is;}
            try{
                size_t start=0;
                bool n=false;
                if(s[start]=='-'||s[start]=='+'){
                    n=(s[start]=='-');
                    start++;
                }
                if(start+1<s.size()&&s[start]=='0'){
                    char p=s[start+1];
                    if(p=='x'||p=='X')v=bigint::from_hex(s);
                    else if(p=='o'||p=='O'){
                        v=bigint::from_oct_str(n?("-"+s.substr(start+2)):s.substr(start+2));
                    }else v=bigint::from_string(s);
                }else{
                    v=bigint::from_string(s);
                }
            }catch(...){
                is.setstate(ios::failbit);
            }
        }
        return is;
    }
    bigint operator-()const{
        bigint r=*this;
        if(!r.is_zero())r.neg=!r.neg;
        return r;
    }
    bigint operator~()const{return -(*this)-bigint(1);}
    bigint operator+(const bigint&o)const{
        bigint r;
        if(neg==o.neg){
            r.mag=mag;
            mag_add(r.mag,o.mag);
            r.neg=neg;
        }else{
            if(mag_lt(mag,o.mag)){
                r.mag=o.mag;mag_sub(r.mag,mag);r.neg=o.neg;
            }else{
                r.mag=mag;mag_sub(r.mag,o.mag);r.neg=neg;
            }
        }
        r.normalize();
        return r;
    }
    bigint operator-(const bigint&o)const{return *this+(-o);}
    bigint operator*(const bigint&o)const{
        bigint r;
        size_t na=mag.size(),nb=o.mag.size();
        size_t nmin=min(na,nb),nmax=max(na,nb);
        if(nb==1)mag_mul_limb_into(view_t(mag),o.mag[0],r.mag);
        else if(na==1)mag_mul_limb_into(view_t(o.mag),mag[0],r.mag);
        else if(nmin<=SMALL_LIMB_THRESHOLD)
            mag_mul_into_4way(view_t(mag),view_t(o.mag),r.mag);
        else if(nmax>=KARATSUBA_THRESHOLD){
            Scratch sc;
            mag_mul_karatsuba_v(view_t(mag),view_t(o.mag),r.mag,sc);
        }else mag_mul_into_4way(view_t(mag),view_t(o.mag),r.mag);
        r.neg=(neg!=o.neg)&&!r.mag.empty();
        r.normalize();
        return r;
    }
    bigint sqr()const{
        bigint r;
        mag_sqr_into(view_t(mag),r.mag);
        r.normalize();
        return r;
    }
    bigint pow(const bigint&exp)const{
        if(exp<0)throw domain_error("bigint: negative exponent");
        bigint res(1),b=*this,e=exp;
        while(!e.is_zero()){
            if(e.is_odd())res*=b;
            b=b.sqr();
            e>>=1;
        }
        return res;
    }
    bigint fact()const{
        if(neg)throw domain_error("bigint: factorial of negative number");
        if(is_zero())return bigint(1);
        long long n=to_ll();
        bigint res(1);
        for(long long i=2;i<=n;++i)res*=bigint(i);
        return res;
    }
    static void divmod(const bigint&x,const bigint&y,bigint&q,bigint&r){
        if(y.is_zero())throw domain_error("bigint: divide by zero");
        bigint a=x;a.neg=false;
        bigint b=y;b.neg=false;
        mag_divmod(view_t(a.mag),view_t(b.mag),q.mag,r.mag);
        q.neg=(x.neg!=y.neg)&&!q.is_zero();
        r.neg=x.neg&&!r.is_zero();
        q.normalize();r.normalize();
    }
    bigint operator/(const bigint&o)const{
        bigint q,r;divmod(*this,o,q,r);return q;
    }
    bigint operator%(const bigint&o)const{
        bigint q,r;divmod(*this,o,q,r);return r;
    }
    bigint operator<<(size_t k)const{
        bigint r=*this;mag_shl(r.mag,k);return r;
    }
    bigint operator>>(size_t k)const{
        if(k==0)return *this;
        if(!neg){bigint r=*this;mag_shr(r.mag,k);return r;}
        bigint t=-(*this);
        t=t-bigint(1);
        mag_shr(t.mag,k);
        return -(t+bigint(1));
    }
    bigint operator&(const bigint&o)const{
        size_t W=work_width(o.mag.size());
        mag_t a=to_twos(*this,W),b=to_twos(o,W);
        for(size_t i=0;i<W;++i)a[i]&=b[i];
        return from_twos(move(a));
    }
    bigint operator|(const bigint&o)const{
        size_t W=work_width(o.mag.size());
        mag_t a=to_twos(*this,W),b=to_twos(o,W);
        for(size_t i=0;i<W;++i)a[i]|=b[i];
        return from_twos(move(a));
    }
    bigint operator^(const bigint&o)const{
        size_t W=work_width(o.mag.size());
        mag_t a=to_twos(*this,W),b=to_twos(o,W);
        for(size_t i=0;i<W;++i)a[i]^=b[i];
        return from_twos(move(a));
    }
    bigint& operator+=(const bigint&o){*this=*this+o;return *this;}
    bigint& operator-=(const bigint&o){*this=*this-o;return *this;}
    bigint& operator*=(const bigint&o){*this=*this*o;return *this;}
    bigint& operator/=(const bigint&o){*this=*this/o;return *this;}
    bigint& operator%=(const bigint&o){*this=*this%o;return *this;}
    bigint& operator&=(const bigint&o){*this=*this&o;return *this;}
    bigint& operator|=(const bigint&o){*this=*this|o;return *this;}
    bigint& operator^=(const bigint&o){*this=*this^o;return *this;}
    bigint& operator<<=(size_t k){*this=*this<<k;return *this;}
    bigint& operator>>=(size_t k){*this=*this>>k;return *this;}
    bigint& operator++(){*this+=bigint(1);return *this;}
    bigint operator++(int){bigint t=*this;++(*this);return t;}
    bigint& operator--(){*this-=bigint(1);return *this;}
    bigint operator--(int){bigint t=*this;--(*this);return t;}
    bool operator==(const bigint&o)const{return neg==o.neg&&mag==o.mag;}
    bool operator!=(const bigint&o)const{return !(*this==o);}
    bool operator<(const bigint&o)const{
        if(neg!=o.neg)return neg;
        if(neg)return mag_lt(o.mag,mag);
        return mag_lt(mag,o.mag);
    }
    bool operator<=(const bigint&o)const{return!(o<*this);}
    bool operator>(const bigint&o)const{return o<*this;}
    bool operator>=(const bigint&o)const{return!(*this<o);}
    bool fits_ll()const{
        if(mag.size()>1)return false;
        if(mag.empty())return true;
        limb_t v=mag[0];
        if(!neg)return v<(limb_t(1)<<63);
        return v<=(limb_t(1)<<63);
    }
    long long to_ll()const{
        assert(fits_ll()&&"bigint: to_ll truncates");
        limb_t v=mag.empty()?0:mag[0];
        return neg?(long long)(limb_t(0)-v):(long long)v;
    }
    string to_string()const{
        if(is_zero())return"0";
        bigint t=*this;t.neg=false;
        bigint base=from_limb(BASE10_19);
        vector<limb_t>chunks;
        while(!t.is_zero()){
            bigint q,r;
            divmod(t,base,q,r);
            chunks.push_back(r.mag.empty()?0:r.mag[0]);
            t=move(q);
        }
        string out=std::to_string(chunks.back());
        for(size_t i=chunks.size()-1;i-->0;){
            char buf[32];
            snprintf(buf,sizeof(buf),"%019llu",
                          static_cast<unsigned long long>(chunks[i]));
            out+=buf;
        }
        return neg?"-"+out:out;
    }
    string to_hex()const{
        if(is_zero())return"0";
        char buf[17];
        string s;
        for(size_t i=mag.size();i-->0;){
            snprintf(buf,sizeof(buf),"%016llx",
                          static_cast<unsigned long long>(mag[i]));
            s+=buf;
        }
        size_t pos=s.find_first_not_of('0');
        string h=(pos==string::npos)?"0":s.substr(pos);
        return neg?"-"+h:h;
    }
    friend ostream& operator<<(ostream&os,const bigint&v){
        return os<<v.to_string();
    }
};
inline bigint pow(const bigint&base,const bigint&exp){
    return base.pow(exp);
}
inline bigint fact(const bigint&n){
    return n.fact();
}
inline bigint sqrt(const bigint&n){
    if(n<bigint(0))throw domain_error("bigint: sqrt of negative number");
    if(n.is_zero())return bigint(0);
    bigint x=n,y=(x+bigint(1))>>1;
    while(y<x){
        x=y;
        y=(x+n/x)>>1;
    }
    return x;
}
static void self_test(){
    using B=bigint;
    assert(B::from_string("123")+B::from_string("456")==B::from_string("579"));
    assert(B::from_string("-100")*B::from_string("3")==B::from_string("-300"));
    assert(B::from_string("100")/B::from_string("7")==B::from_string("14"));
    assert(B::from_string("100")%B::from_string("7")==B::from_string("2"));
    assert(B::from_hex("0xFF")==B(255));
    assert(B::from_oct_str("0o55")==B(45));
    assert(fact(5)==B(120));
    assert(pow(B(2),10)==B(1024));
    assert(B(20).fact()==fact(B(20)));
    assert(sqrt(B(144))==B(12));
    B x=(B(1)<<4096)-B(1);
    B expect=(B(1)<<8192)-(B(1)<<4097)+B(1);
    assert(x*x==expect);
    cout<<"[self_test] all passed\n";
}
int main(){
    
    return 0;
}
